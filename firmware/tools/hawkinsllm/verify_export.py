#!/usr/bin/env python3
"""Validate ROZA CRDP-v3 GPT-Neo weights and CTK2 tokenizer before embedding."""

from __future__ import annotations
import argparse
import json
import struct
from pathlib import Path

PARTITION_BYTES = 0x7E0000
MARGIN_BYTES = 256 * 1024
MODEL_MAGIC = b"CRDP"
TOKEN_MAGIC = 0x324B5443
GPT_NEO_ARCH = 2
Q4_0 = 4


def row_stride(cols: int) -> int:
    if cols <= 0 or cols % 32:
        raise ValueError(f"dimension {cols} must be positive and divisible by 32")
    blocks = cols // 32
    scale_bytes = 2 * blocks
    pad = (-scale_bytes) % 16
    return scale_bytes + pad + 16 * blocks


def verify_model(path: Path) -> dict:
    data = path.read_bytes()
    if len(data) < 64 or data[:4] != MODEL_MAGIC:
        raise ValueError("CRDP header missing or truncated")
    version, = struct.unpack_from("<I", data, 4)
    if version != 3:
        raise ValueError(f"CRDP version must be 3, got {version}")
    dim, hidden, layers, heads, kv_heads, vocab, seq_len = struct.unpack_from("<7i", data, 8)
    shared, quant, arch = struct.unpack_from("<BBB", data, 36)
    if arch != GPT_NEO_ARCH or quant != Q4_0:
        raise ValueError(f"expected GPT-Neo Q4_0, got arch={arch} quant={quant}")
    if not (0 < layers <= 32 and 0 < heads <= 32 and 0 < vocab <= 32768):
        raise ValueError("model dimensions/layers/vocabulary outside MCU guardrails")
    if dim % heads or dim % 32 or hidden % 32 or seq_len < 32 or seq_len > 80:
        raise ValueError("dimensions or context window incompatible with Hawkins profile")
    if kv_heads != heads:
        raise ValueError("GPT-Neo requires n_kv_heads == n_heads")
    if shared != 1:
        raise ValueError("TinyTalk export must use tied embeddings/classifier")

    ld = layers * dim
    float_count = 6 * ld + 2 * dim + layers * hidden + seq_len * dim
    offset = 64 + float_count * 4
    offset = (offset + 15) & ~15
    offset += vocab * row_stride(dim)                    # wte
    offset += layers * (4 * dim * row_stride(dim))       # q, k, v, out
    offset += layers * (hidden * row_stride(dim))        # MLP c_fc
    offset += layers * (dim * row_stride(hidden))        # MLP c_proj
    if offset != len(data):
        raise ValueError(f"CRDP size mismatch: computed {offset}, file has {len(data)} bytes")
    return {
        "file": path.name, "bytes": len(data), "version": version, "architecture": "gpt_neo",
        "dimension": dim, "hidden_dimension": hidden, "layers": layers, "heads": heads,
        "vocabulary": vocab, "position_context": seq_len, "quantization": "Q4_0",
    }


def verify_tokenizer(path: Path, expected_vocab: int) -> dict:
    data = path.read_bytes()
    if len(data) < 20 + 512:
        raise ValueError("CTK2 blob too short")
    magic, vocab, max_len, eos, merges = struct.unpack_from("<Iiiii", data, 0)
    if magic != TOKEN_MAGIC:
        raise ValueError("CTK2 magic mismatch")
    if vocab != expected_vocab or not (256 <= vocab <= 32768):
        raise ValueError(f"tokenizer vocabulary {vocab} does not match model {expected_vocab}")
    if max_len <= 0 or max_len > 512 or eos < 0 or eos >= vocab or merges < 0:
        raise ValueError("invalid CTK2 header")
    offset = 20
    byte_ids = struct.unpack_from("<256H", data, offset)
    offset += 512
    if any(token_id >= vocab for token_id in byte_ids):
        raise ValueError("a byte fallback token id is outside the pruned vocabulary")
    merge_bytes = merges * 6
    if offset + merge_bytes > len(data):
        raise ValueError("truncated CTK2 merge table")
    triples = [struct.unpack_from("<HHH", data, offset + i * 6) for i in range(merges)]
    if any(a >= vocab or b >= vocab or c >= vocab for a, b, c in triples):
        raise ValueError("CTK2 merge references a token outside the vocabulary")
    if triples != sorted(triples, key=lambda item: (item[0], item[1])):
        raise ValueError("CTK2 merge table is not sorted for binary search")
    offset += merge_bytes
    largest_piece = 0
    for token_id in range(vocab):
        if offset + 4 > len(data):
            raise ValueError(f"truncated CTK2 token record at id {token_id}")
        length, = struct.unpack_from("<i", data, offset)
        offset += 4
        if length < 0 or length > max_len or offset + length > len(data):
            raise ValueError(f"invalid CTK2 token length at id {token_id}")
        largest_piece = max(largest_piece, length)
        offset += length
    if offset != len(data):
        raise ValueError(f"unexpected trailing bytes in CTK2 blob: {len(data) - offset}")
    if largest_piece != max_len:
        raise ValueError(f"CTK2 max token length mismatch: header {max_len}, observed {largest_piece}")
    return {
        "file": path.name, "bytes": len(data), "magic": "CTK2", "vocabulary": vocab,
        "eos_id": eos, "merges": merges, "max_token_bytes": max_len,
        "byte_fallback_entries": len(byte_ids),
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", required=True)
    ap.add_argument("--tokenizer", required=True)
    ap.add_argument("--firmware-bin", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--partition-bytes", type=int, default=PARTITION_BYTES)
    ap.add_argument("--margin-bytes", type=int, default=MARGIN_BYTES)
    args = ap.parse_args()

    model = verify_model(Path(args.model))
    tokenizer = verify_tokenizer(Path(args.tokenizer), model["vocabulary"])
    firmware_size = Path(args.firmware_bin).stat().st_size
    total = firmware_size + model["bytes"] + tokenizer["bytes"]
    limit = args.partition_bytes - args.margin_bytes
    if total > limit:
        raise SystemExit(
            f"Flash budget exceeded: base firmware + model + tokenizer = {total:,} bytes; "
            f"allowed with safety margin = {limit:,} bytes"
        )
    report = {
        "status": "valid",
        "model": model,
        "tokenizer": tokenizer,
        "base_firmware_bytes": firmware_size,
        "estimated_embedded_total_bytes": total,
        "application_partition_bytes": args.partition_bytes,
        "reserved_safety_margin_bytes": args.margin_bytes,
        "remaining_after_reserve_bytes": limit - total,
        "note": "Estimate before stage-4 integration; final linked firmware size must be measured again.",
    }
    Path(args.output).write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                                 encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
