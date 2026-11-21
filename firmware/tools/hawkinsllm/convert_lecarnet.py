#!/usr/bin/env python3
"""Export MaxLSB/LeCarnet LLaMA weights to ROZA CRDP v3 + CTK2 byte-BPE."""

from __future__ import annotations
import argparse
import collections
import json
import struct
from pathlib import Path

import numpy as np

MAGIC = b"CRDP"
FORMAT_V3 = 3
ARCH_LLAMA = 1
Q4_0 = 4
BLOCK = 32
TOK_MAGIC = 0x324B5443


def bytes_to_unicode():
    bs = list(range(ord("!"), ord("~") + 1))
    bs += list(range(ord("\xa1"), ord("\xac") + 1))
    bs += list(range(ord("\xae"), ord("\xff") + 1))
    cs = bs[:]
    n = 0
    for b in range(256):
        if b not in bs:
            bs.append(b)
            cs.append(256 + n)
            n += 1
    return dict(zip(bs, [chr(c) for c in cs]))


def piece_to_bytes(piece: str, unicode_to_byte: dict[str, int]) -> bytes:
    return bytes(unicode_to_byte[ch] for ch in piece)


def row_stride(cols: int) -> int:
    if cols <= 0 or cols % BLOCK:
        raise ValueError(f"Q4 row width {cols} must be divisible by {BLOCK}")
    groups = cols // BLOCK
    scale_bytes = 2 * groups
    pad = (-scale_bytes) % 16
    return scale_bytes + pad + 16 * groups


def quantize_q4_v3(matrix: np.ndarray, cols: int) -> bytes:
    rows = np.ascontiguousarray(matrix, dtype=np.float32).reshape(-1, cols)
    if not np.isfinite(rows).all():
        raise ValueError("model weights contain NaN or infinity")
    n_rows, _ = rows.shape
    groups = cols // BLOCK
    blocks = rows.reshape(n_rows, groups, BLOCK)
    imax = np.abs(blocks).argmax(axis=2)
    ridx = np.arange(n_rows)[:, None]
    gidx = np.arange(groups)[None, :]
    peak = blocks[ridx, gidx, imax]
    scale = peak / -8.0
    inv = np.where(scale != 0, 1.0 / np.where(scale != 0, scale, 1.0), 0.0)
    q = np.clip(np.floor(blocks * inv[:, :, None] + 8.5), 0, 15).astype(np.uint8)
    f32 = scale.astype(np.float32).view(np.uint32)
    bf16 = ((f32 + 0x7FFF + ((f32 >> 16) & 1)) >> 16).astype("<u2")
    packed = (q[:, :, :16] & 0x0F) | ((q[:, :, 16:] & 0x0F) << 4)
    sbytes = 2 * groups
    pad = (-sbytes) % 16
    out = np.zeros((n_rows, row_stride(cols)), dtype=np.uint8)
    out[:, :sbytes] = bf16.view(np.uint8).reshape(n_rows, sbytes)
    out[:, sbytes + pad:] = packed.reshape(n_rows, 16 * groups)
    return out.tobytes()


def prune_bpe(snapshot: Path, corpus_files: list[Path], eos_id: int,
              min_count: int, max_vocab: int):
    # LeCarnet inherits PAGnol's GPT-2 vocabulary. Its first six special-token
    # ids all have the empty spelling, so preserve duplicate JSON keys.
    pairs = json.loads(
        (snapshot / "vocab.json").read_text(encoding="utf-8"),
        object_pairs_hook=lambda items: items,
    )
    vocab = {}
    id_to_piece = {}
    for piece, raw_id in pairs:
        old_id = int(raw_id)
        id_to_piece[old_id] = piece
        if piece:
            if piece in vocab:
                raise ValueError(f"duplicate non-empty BPE token {piece!r}")
            vocab[piece] = old_id
    if eos_id not in id_to_piece or id_to_piece[eos_id] != "":
        raise ValueError(f"LeCarnet special EOS id {eos_id} is not present")
    b2u = bytes_to_unicode()
    u2b = {v: k for k, v in b2u.items()}

    byte_old = {}
    for old_id, piece in id_to_piece.items():
        try:
            raw = piece_to_bytes(piece, u2b)
        except KeyError:
            continue
        if len(raw) == 1:
            if raw[0] in byte_old:
                raise ValueError(f"duplicate byte-level base token for byte {raw[0]}")
            byte_old[raw[0]] = old_id
    if len(byte_old) != 256:
        raise ValueError(f"GPT-2 tokenizer must have all 256 byte tokens; found {len(byte_old)}")

    from transformers import AutoTokenizer
    tokenizer = AutoTokenizer.from_pretrained(snapshot, use_fast=True)
    counts = collections.Counter()
    for path in corpus_files:
        if not path.exists():
            raise FileNotFoundError(path)
        text = path.read_text(encoding="utf-8")
        chunk_size = 500_000
        for start in range(0, len(text), chunk_size):
            counts.update(tokenizer.encode(text[start:start + chunk_size], add_special_tokens=False))

    lines = (snapshot / "merges.txt").read_text(encoding="utf-8").splitlines()
    if lines and lines[0].startswith("#"):
        lines = lines[1:]
    created_by: dict[int, tuple[int, int]] = {}
    rank_by_id: dict[int, int] = {}
    for rank, line in enumerate(lines):
        parts = line.split()
        if len(parts) != 2:
            continue
        a, b = parts
        rid = vocab.get(a + b)
        if rid is None:
            raise ValueError(f"merge rank {rank} result {a+b!r} missing from vocab.json")
        rid = int(rid)
        aid, bid = int(vocab[a]), int(vocab[b])
        if rid in created_by:
            raise ValueError(f"duplicate merge result token id {rid}")
        created_by[rid] = (aid, bid)
        rank_by_id[rid] = rank

    keep = set(byte_old.values())
    keep.add(eos_id)
    keep.update(tid for tid, count in counts.items() if count >= min_count)
    stack = list(keep)
    while stack:
        token_id = stack.pop()
        parents = created_by.get(token_id)
        if parents:
            for parent in parents:
                if parent not in keep:
                    keep.add(parent)
                    stack.append(parent)

    # Put all byte fallback tokens first, then merge results in exact BPE rank
    # order, then special/non-merge tokens. This preserves the runtime's rule
    # that the lowest output id corresponds to the earliest applicable merge.
    ordered = [byte_old[b] for b in range(256)]
    ordered += [tid for tid, _ in sorted(rank_by_id.items(), key=lambda item: item[1])
                if tid in keep and tid not in set(ordered)]
    ordered_set = set(ordered)
    extras = sorted(tid for tid in keep if tid not in ordered_set and tid != eos_id)
    ordered += extras
    if eos_id not in set(ordered):
        ordered.append(eos_id)
    if len(ordered) > max_vocab:
        raise ValueError(
            f"pruned vocab has {len(ordered)} tokens, above --max-vocab {max_vocab}; "
            "increase --min-count or reduce the corpus sample"
        )
    old_to_new = {old: new for new, old in enumerate(ordered)}
    pieces = []
    for old in ordered:
        if old == eos_id:
            pieces.append(b"")
        else:
            pieces.append(piece_to_bytes(id_to_piece[old], u2b))

    merges = []
    for old_result, (old_a, old_b) in created_by.items():
        if old_result in old_to_new and old_a in old_to_new and old_b in old_to_new:
            merges.append((old_to_new[old_a], old_to_new[old_b], old_to_new[old_result]))
    merges.sort(key=lambda item: (item[0], item[1]))
    if len(merges) > 65535:
        raise ValueError("CTK2 merge count exceeds uint16 token index capacity")
    return pieces, merges, old_to_new[eos_id], old_to_new, ordered


def write_ctk2(path: Path, pieces: list[bytes], merges: list[tuple[int, int, int]], eos_id: int):
    max_len = max((len(piece) for piece in pieces), default=0)
    with path.open("wb") as out:
        out.write(struct.pack("<Iiiii", TOK_MAGIC, len(pieces), max_len, eos_id, len(merges)))
        out.write(struct.pack("<256H", *range(256)))
        for a, b, c in merges:
            out.write(struct.pack("<HHH", a, b, c))
        for piece in pieces:
            out.write(struct.pack("<i", len(piece)))
            out.write(piece)


def load_weights(model_dir: Path):
    try:
        from safetensors.torch import load_file
    except ImportError as exc:
        raise SystemExit("Install safetensors to load the checkpoint") from exc
    files = sorted(model_dir.glob("*.safetensors"))
    if files:
        state = {}
        for path in files:
            state.update(load_file(str(path), device="cpu"))
        return {key: tensor.float().cpu().numpy() for key, tensor in state.items()}
    import torch
    path = model_dir / "pytorch_model.bin"
    if not path.exists():
        raise FileNotFoundError("No model.safetensors or pytorch_model.bin in checkpoint")
    state = torch.load(path, map_location="cpu", weights_only=True)
    return {key: tensor.float().cpu().numpy() for key, tensor in state.items()}


def export_model(model_dir: Path, output: Path, tokenizer_output: Path,
                 corpus_files: list[Path], context: int, min_count: int,
                 max_vocab: int):
    cfg = json.loads((model_dir / "config.json").read_text(encoding="utf-8"))
    if cfg.get("model_type") != "llama":
        raise ValueError(f"expected LLaMA architecture, got {cfg.get('model_type')}")
    dim = int(cfg["hidden_size"])
    hidden = int(cfg["intermediate_size"])
    layers = int(cfg["num_hidden_layers"])
    heads = int(cfg["num_attention_heads"])
    kv_heads = int(cfg.get("num_key_value_heads", heads))
    vocab_size = int(cfg["vocab_size"])
    eos_old = int(cfg.get("eos_token_id", -1))
    bos_old = int(cfg.get("bos_token_id", eos_old))
    shared = bool(cfg.get("tie_word_embeddings", False))
    if dim % heads or dim % BLOCK or hidden % BLOCK or kv_heads != heads:
        raise ValueError("ROZA v3 LLaMA requires dim/hidden divisible by 32 and full multi-head attention")
    if context < 16 or context > 80:
        raise ValueError("Cardputer profile supports a context cap between 16 and 80 tokens")
    if bos_old != eos_old:
        raise ValueError("CTK2 v1 profile requires LeCarnet BOS and EOS to use the same token id")
    if not shared:
        raise ValueError("This compact profile expects tied LLaMA embeddings/classifier")

    state = load_weights(model_dir)
    required = [
        "model.embed_tokens.weight", "model.norm.weight",
        "model.layers.0.input_layernorm.weight",
        "model.layers.0.post_attention_layernorm.weight",
        "model.layers.0.self_attn.q_proj.weight",
        "model.layers.0.self_attn.k_proj.weight",
        "model.layers.0.self_attn.v_proj.weight",
        "model.layers.0.self_attn.o_proj.weight",
        "model.layers.0.mlp.gate_proj.weight",
        "model.layers.0.mlp.up_proj.weight",
        "model.layers.0.mlp.down_proj.weight",
    ]
    missing = [key for key in required if key not in state]
    if missing:
        raise ValueError(f"checkpoint tensor naming mismatch: {missing}")
    embed = state["model.embed_tokens.weight"]
    if embed.shape != (vocab_size, dim):
        raise ValueError(f"embedding shape {embed.shape} does not match config {(vocab_size, dim)}")
    if int(cfg.get("eos_token_id", -1)) != eos_old:
        raise ValueError("invalid EOS id")

    # Use the saved tokenizer files in the fine-tuned checkpoint. The 3M model
    # uses GPT-2 byte-level BPE; CTK2 stores the exact reduced vocabulary.
    pieces, merges, eos_new, old_to_new, ordered = prune_bpe(
        model_dir, corpus_files, eos_old, min_count, max_vocab
    )
    pruned_vocab = len(pieces)
    if len(ordered) != pruned_vocab or set(old_to_new.values()) != set(range(pruned_vocab)):
        raise ValueError("tokenizer pruning produced inconsistent vocabulary")

    def layer(l: int, name: str):
        key = f"model.layers.{l}.{name}"
        if key not in state:
            raise KeyError(f"missing model tensor {key}")
        return state[key]

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as out:
        out.write(MAGIC)
        out.write(struct.pack("<I", FORMAT_V3))
        out.write(struct.pack("<iiiiiii", dim, hidden, layers, heads, kv_heads,
                              pruned_vocab, context))
        out.write(struct.pack("<BBB", 1, Q4_0, ARCH_LLAMA))
        out.write(b"\x00" * (64 - out.tell()))

        for tensor in (
            np.stack([layer(i, "input_layernorm.weight") for i in range(layers)]),
            np.stack([layer(i, "post_attention_layernorm.weight") for i in range(layers)]),
            state["model.norm.weight"],
        ):
            out.write(np.ascontiguousarray(tensor, dtype=np.float32).tobytes())

        out.write(b"\x00" * ((-out.tell()) % 16))
        new_to_old = np.asarray(ordered, dtype=np.int64)
        out.write(quantize_q4_v3(embed[new_to_old], dim))

        projections = [
            ("self_attn.q_proj.weight", dim),
            ("self_attn.k_proj.weight", dim),
            ("self_attn.v_proj.weight", dim),
            ("self_attn.o_proj.weight", dim),
            ("mlp.gate_proj.weight", dim),
            ("mlp.down_proj.weight", hidden),
            ("mlp.up_proj.weight", dim),
        ]
        for suffix, width in projections:
            stacked = np.stack([layer(i, suffix) for i in range(layers)])
            expected_rows = hidden if suffix in ("mlp.gate_proj.weight", "mlp.up_proj.weight") else dim
            if stacked.shape != (layers, expected_rows, width):
                raise ValueError(
                    f"{suffix} shape mismatch: {stacked.shape}; "
                    f"expected {(layers, expected_rows, width)}"
                )
            out.write(quantize_q4_v3(stacked, width))

        # Tied embeddings: classifier is an alias of the compact token table.
        final_size = out.tell()
        model_size = final_size
        if model_size > 0x7E0000:
            raise ValueError(f"model alone exceeds app partition: {model_size} bytes")

    write_ctk2(tokenizer_output, pieces, merges, eos_new)
    print(f"[+] LLaMA CRDP v3: {model_size:,} bytes; vocabulary={pruned_vocab}")
    print(f"[+] CTK2: {tokenizer_output.stat().st_size:,} bytes; eos_id={eos_new}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model-dir", required=True)
    ap.add_argument("--corpus", action="append", default=[], required=True)
    ap.add_argument("--output-model", required=True)
    ap.add_argument("--output-tokenizer", required=True)
    ap.add_argument("--context", type=int, default=32)
    ap.add_argument("--min-count", type=int, default=2)
    ap.add_argument("--max-vocab", type=int, default=8000)
    args = ap.parse_args()
    export_model(
        Path(args.model_dir),
        Path(args.output_model),
        Path(args.output_tokenizer),
        [Path(path) for path in args.corpus],
        args.context, args.min_count, args.max_vocab,
    )


if __name__ == "__main__":
    main()
