#!/usr/bin/env python3
"""Verify LeCarnet LLaMA CRDP-v3 row-planar weights and CTK2 byte-BPE."""

from __future__ import annotations
import argparse
import json
import struct
from pathlib import Path

PARTITION_BYTES = 0x7E0000
SAFETY_MARGIN = 256 * 1024
MAGIC = b"CRDP"
TOK_MAGIC = 0x324B5443
VERSION = 3
ARCH_LLAMA = 1
Q4_0 = 4
BLOCK = 32


def row_stride(cols: int) -> int:
    if cols <= 0 or cols % BLOCK:
        raise ValueError(f"dimension {cols} is not a multiple of {BLOCK}")
    groups = cols // BLOCK
    scale_bytes = 2 * groups
    return scale_bytes + ((-scale_bytes) % 16) + 16 * groups


def verify_model(path: Path):
    data = path.read_bytes()
    if len(data) < 64 or data[:4] != MAGIC:
        raise ValueError("missing CRDP header")
    version, = struct.unpack_from("<I", data, 4)
    if version != VERSION:
        raise ValueError(f"CRDP version must be {VERSION}, got {version}")
    dim, hidden, layers, heads, kv_heads, vocab, context = struct.unpack_from("<7i", data, 8)
    shared, quant, arch = struct.unpack_from("<BBB", data, 36)
    if arch != ARCH_LLAMA or quant != Q4_0 or shared != 1:
        raise ValueError(f"expected shared LLaMA Q4_0, got arch={arch}, quant={quant}, shared={shared}")
    if not (0 < layers <= 16 and 0 < vocab <= 8000 and 16 <= context <= 64):
        raise ValueError("model exceeds Cardputer dimensions/context guardrails")
    if dim % heads or dim % BLOCK or hidden % BLOCK or kv_heads != heads:
        raise ValueError("LLaMA dimensions are incompatible with the ROZA v3 kernel")
    off = 64 + (2 * layers * dim + dim) * 4
    off = (off + 15) & ~15
    off += vocab * row_stride(dim)                     # embedding, tied classifier
    off += layers * dim * row_stride(dim) * 4          # Q, K, V, O
    off += layers * hidden * row_stride(dim)           # gate
    off += layers * dim * row_stride(hidden)           # down
    off += layers * hidden * row_stride(dim)           # up
    if off != len(data):
        raise ValueError(f"CRDP size mismatch: expected {off}, got {len(data)}")
    return {
        "bytes": len(data), "version": version, "architecture": "llama",
        "dim": dim, "hidden": hidden, "layers": layers, "heads": heads,
        "vocabulary": vocab, "context": context, "quantization": "Q4_0 row-planar",
    }


def verify_tokenizer(path: Path, expected_vocab: int):
    data = path.read_bytes()
    if len(data) < 20 + 512:
        raise ValueError("CTK2 blob too short")
    magic, vocab, max_len, eos, merges = struct.unpack_from("<Iiiii", data, 0)
    if magic != TOK_MAGIC or vocab != expected_vocab:
        raise ValueError("CTK2 header or vocabulary mismatch")
    if not (256 <= vocab <= 8000 and 256 <= eos < vocab and 1 <= max_len <= 512 and merges >= 0):
        raise ValueError("invalid CTK2 dimensions (EOS must be a non-byte special token)")
    offset = 20
    byte_ids = struct.unpack_from("<256H", data, offset)
    offset += 512
    if tuple(byte_ids) != tuple(range(256)):
        raise ValueError("CTK2 byte fallback ids must map to ids 0..255")
    if offset + merges * 6 > len(data):
        raise ValueError("truncated CTK2 merge table")
    triples = [struct.unpack_from("<HHH", data, offset + i * 6) for i in range(merges)]
    if triples != sorted(triples, key=lambda item: (item[0], item[1])):
        raise ValueError("CTK2 merge table is not sorted by pair")
    if any(a >= vocab or b >= vocab or c >= vocab for a, b, c in triples):
        raise ValueError("CTK2 merge references an invalid token")
    if any(c < 256 or a >= c or b >= c for a, b, c in triples):
        raise ValueError("CTK2 merge ids do not preserve byte-BPE rank order")
    if len({c for _, _, c in triples}) != len(triples):
        raise ValueError("CTK2 contains duplicate merge-result ids")
    offset += merges * 6
    largest = 0
    eos_length = None
    for token_id in range(vocab):
        if offset + 4 > len(data):
            raise ValueError(f"truncated token {token_id}")
        length, = struct.unpack_from("<i", data, offset)
        offset += 4
        if length < 0 or length > max_len or offset + length > len(data):
            raise ValueError(f"invalid token length for token {token_id}")
        if token_id == eos:
            eos_length = length
        largest = max(largest, length)
        offset += length
    if eos_length != 0:
        raise ValueError("CTK2 EOS must be a non-text special token with an empty byte piece")
    if offset != len(data) or largest != max_len:
        raise ValueError("CTK2 length or trailing-data check failed")
    return {
        "bytes": len(data), "format": "CTK2 byte-level BPE",
        "vocabulary": vocab, "eos_id": eos, "merges": merges,
        "max_token_bytes": max_len,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", required=True)
    ap.add_argument("--tokenizer", required=True)
    ap.add_argument("--firmware-bin", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--partition-bytes", type=int, default=PARTITION_BYTES)
    ap.add_argument("--margin-bytes", type=int, default=SAFETY_MARGIN)
    args = ap.parse_args()
    model = verify_model(Path(args.model))
    tokenizer = verify_tokenizer(Path(args.tokenizer), model["vocabulary"])
    firmware_bytes = Path(args.firmware_bin).stat().st_size
    total = firmware_bytes + model["bytes"] + tokenizer["bytes"]
    allowed = args.partition_bytes - args.margin_bytes

    # Approximate dynamic heap requested by llm_init_embedded for the current
    # LLaMA runtime: six dim vectors, two MLP vectors, Q/K/V, attention,
    # logits, BF16 K/V cache and v3 Q8 activation buffers.
    dim = model["dim"]
    hidden = model["hidden"]
    layers = model["layers"]
    heads = model["heads"]
    context = model["context"]
    kv_dim = dim  # This profile requires full multi-head attention.
    float_state = (6 * dim + 2 * hidden + heads * context + model["vocabulary"]) * 4
    kv_cache = 2 * layers * context * kv_dim * 2  # K and V, BF16
    q8_scratch = max(dim, hidden) + (max(dim, hidden) // 32) * 4
    estimated_llm_heap = float_state + kv_cache + q8_scratch
    if total > allowed:
        raise SystemExit(
            f"Flash budget exceeded: {total:,} bytes > {allowed:,} bytes "
            "after reserved safety margin"
        )
    report = {
        "status": "valid",
        "model": model,
        "tokenizer": tokenizer,
        "base_firmware_bytes": firmware_bytes,
        "estimated_embedded_total_bytes": total,
        "application_partition_bytes": args.partition_bytes,
        "safety_margin_bytes": args.margin_bytes,
        "remaining_after_margin_bytes": allowed - total,
        "estimated_llm_dynamic_heap_bytes": estimated_llm_heap,
        "estimated_llm_dynamic_heap_kib": round(estimated_llm_heap / 1024, 2),
        "runtime_heap_note": "Arithmetic estimate from llm_init_embedded allocations; not a measurement of free heap on the physical Cardputer. Stage 5 must log heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) before/after init.",
        "note": "Estimation before stage-4 embedding; final binary size and runtime heap must be remeasured.",
    }
    Path(args.output).write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
