#!/usr/bin/env python3
"""Fine-tune TheREZOR/TinyTalk on short French chat pairs for an 80-token MCU window."""

from __future__ import annotations
import argparse
import json
import math
import random
import shutil
import time
from pathlib import Path

import torch
from torch.utils.data import DataLoader
from transformers import AutoModelForCausalLM, AutoTokenizer
from huggingface_hub import snapshot_download


def read_jsonl(path: Path) -> list[dict[str, str]]:
    rows = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            rows.append(json.loads(line))
    return rows


def encode_pair(row: dict[str, str], tokenizer, max_length: int) -> dict[str, list[int]]:
    # Format exact utilisé par TinyTalk et reproduit sur le Cardputer.
    prefix = tokenizer.encode(f"User: {row['prompt']}\nBot:", add_special_tokens=False)
    reply = tokenizer.encode(" " + row["response"], add_special_tokens=False)
    eos = tokenizer.eos_token_id
    if eos is None:
        raise RuntimeError("Le tokenizer de base n'a pas de token EOS.")
    bos = tokenizer.bos_token_id
    if bos is not None:
        prefix = [bos] + prefix
    max_prefix = max(8, max_length - 10)
    if len(prefix) > max_prefix:
        if bos is not None:
            prefix = [prefix[0]] + prefix[-(max_prefix - 1):]
        else:
            prefix = prefix[-max_prefix:]
    available = max_length - len(prefix)
    if available < 2:
        raise RuntimeError("Le préfixe consomme toute la fenêtre de contexte.")
    if len(reply) + 1 > available:
        reply = reply[:available - 1]
    target = reply + [eos]
    ids = prefix + target
    labels = [-100] * len(prefix) + target
    return {"input_ids": ids, "labels": labels}


def collate(rows: list[dict[str, list[int]]], pad_id: int) -> dict[str, torch.Tensor]:
    length = max(len(row["input_ids"]) for row in rows)
    input_ids, labels, attention = [], [], []
    for row in rows:
        missing = length - len(row["input_ids"])
        input_ids.append(row["input_ids"] + [pad_id] * missing)
        labels.append(row["labels"] + [-100] * missing)
        attention.append([1] * len(row["input_ids"]) + [0] * missing)
    return {
        "input_ids": torch.tensor(input_ids, dtype=torch.long),
        "labels": torch.tensor(labels, dtype=torch.long),
        "attention_mask": torch.tensor(attention, dtype=torch.long),
    }


@torch.no_grad()
def evaluate(model, loader, device) -> float:
    model.eval()
    total_loss, total_tokens = 0.0, 0
    for batch in loader:
        batch = {key: value.to(device) for key, value in batch.items()}
        active = int((batch["labels"] != -100).sum().item())
        result = model(**batch)
        if not torch.isfinite(result.loss):
            raise RuntimeError("Perte de validation non finie.")
        total_loss += float(result.loss.item()) * active
        total_tokens += active
    model.train()
    if not total_tokens:
        raise RuntimeError("La validation ne contient aucun token de réponse.")
    return total_loss / total_tokens


@torch.no_grad()
def generate_sample(model, tokenizer, prompt: str, device: str) -> str:
    model.eval()
    encoded = tokenizer(f"User: {prompt}\nBot:", return_tensors="pt",
                         add_special_tokens=False).input_ids
    if tokenizer.bos_token_id is not None:
        bos = torch.tensor([[tokenizer.bos_token_id]], dtype=torch.long)
        encoded = torch.cat((bos, encoded), dim=1)
    ids = encoded.to(device)
    # EOS is also used as PAD for this compact tokenizer, so generation must
    # receive an explicit mask rather than trying to infer it from token IDs.
    attention_mask = torch.ones_like(ids)
    out = model.generate(
        ids, attention_mask=attention_mask, max_new_tokens=28, do_sample=False,
        eos_token_id=tokenizer.eos_token_id,
        pad_token_id=tokenizer.eos_token_id,
        use_cache=True,
    )
    text = tokenizer.decode(out[0, ids.shape[1]:], skip_special_tokens=True)
    model.train()
    return text.strip()


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--data-dir", required=True)
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--base", default="TheREZOR/TinyTalk")
    ap.add_argument("--base-revision", default="main")
    ap.add_argument("--epochs", type=int, default=3)
    ap.add_argument("--batch-size", type=int, default=8)
    ap.add_argument("--max-length", type=int, default=80)
    ap.add_argument("--learning-rate", type=float, default=5e-5)
    ap.add_argument("--seed", type=int, default=1703)
    args = ap.parse_args()

    if not 32 <= args.max_length <= 80:
        raise SystemExit("--max-length doit rester entre 32 et 80 pour le profil Cardputer.")
    if args.epochs < 1 or args.batch_size < 1:
        raise SystemExit("--epochs et --batch-size doivent être positifs.")

    random.seed(args.seed)
    torch.manual_seed(args.seed)
    torch.set_num_threads(2)
    device = torch.device("cpu")
    print(f"[+] device={device}, max_length={args.max_length}", flush=True)

    snapshot = snapshot_download(repo_id=args.base, revision=args.base_revision)
    resolved_revision = Path(snapshot).name
    tokenizer = AutoTokenizer.from_pretrained(snapshot, use_fast=True)
    if tokenizer.eos_token_id is None:
        raise RuntimeError("EOS manquant dans le tokenizer de base.")
    tokenizer.pad_token = tokenizer.eos_token

    model = AutoModelForCausalLM.from_pretrained(snapshot).to(device)
    base_architecture = getattr(model.config, "model_type", "")
    if base_architecture not in ("gpt_neo", "llama"):
        raise RuntimeError(
            f"Architecture de base non prise en charge: {base_architecture}; "
            "GPT-Neo ou LLaMA requis."
        )
    model.config.use_cache = False
    parameter_count = sum(parameter.numel() for parameter in model.parameters())
    print(f"[+] architecture={base_architecture} params={parameter_count:,}", flush=True)

    data_dir = Path(args.data_dir)
    train_rows = read_jsonl(data_dir / "train.jsonl")
    val_rows = read_jsonl(data_dir / "validation.jsonl")
    if len(train_rows) < 80 or len(val_rows) < 12:
        raise RuntimeError("Corpus insuffisant après préparation.")

    train_data = [encode_pair(row, tokenizer, args.max_length) for row in train_rows]
    val_data = [encode_pair(row, tokenizer, args.max_length) for row in val_rows]
    collate_fn = lambda batch: collate(batch, tokenizer.eos_token_id)
    generator = torch.Generator().manual_seed(args.seed)
    train_loader = DataLoader(train_data, batch_size=args.batch_size, shuffle=True,
                              generator=generator, collate_fn=collate_fn)
    val_loader = DataLoader(val_data, batch_size=args.batch_size, shuffle=False,
                            collate_fn=collate_fn)

    optimizer = torch.optim.AdamW(model.parameters(), lr=args.learning_rate, weight_decay=0.01)
    total_steps = max(1, len(train_loader) * args.epochs)
    warmup_steps = max(1, int(total_steps * 0.05))

    def lr_scale(step: int) -> float:
        if step < warmup_steps:
            return max(0.05, (step + 1) / warmup_steps)
        progress = (step - warmup_steps) / max(1, total_steps - warmup_steps)
        return max(0.1, 0.5 * (1.0 + math.cos(math.pi * progress)))

    scheduler = torch.optim.lr_scheduler.LambdaLR(optimizer, lr_scale)
    best_loss = float("inf")
    best_state = None
    history = []
    start = time.time()
    model.train()

    print(f"[+] train={len(train_data)} validation={len(val_data)} steps={total_steps}", flush=True)
    for epoch in range(args.epochs):
        running, batches = 0.0, 0
        for batch in train_loader:
            batch = {key: value.to(device) for key, value in batch.items()}
            result = model(**batch)
            loss = result.loss
            if not torch.isfinite(loss):
                raise RuntimeError("Perte d'entraînement non finie.")
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()
            scheduler.step()
            optimizer.zero_grad(set_to_none=True)
            running += float(loss.item())
            batches += 1
        val_loss = evaluate(model, val_loader, device)
        row = {"epoch": epoch + 1, "train_loss_mean": running / max(1, batches),
               "validation_token_loss": val_loss}
        history.append(row)
        print(f"[+] epoch={epoch + 1} train={row['train_loss_mean']:.4f} "
              f"val={val_loss:.4f}", flush=True)
        if val_loss < best_loss:
            best_loss = val_loss
            best_state = {key: value.detach().cpu().clone()
                          for key, value in model.state_dict().items()}

    if best_state is None:
        raise RuntimeError("Aucun checkpoint d'entraînement valide.")
    model.load_state_dict(best_state)
    model.config.use_cache = True

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    model.save_pretrained(out_dir, safe_serialization=False)
    tokenizer.save_pretrained(out_dir)

    # LeCarnet stores its six control-token ids in tokenizer_config.json's
    # added_tokens_decoder rather than in the ordinary GPT-2 vocab/merge table.
    # Restore the pinned source tokenizer metadata after save_pretrained so the
    # exporter can preserve the exact BOS/EOS/PAD/UNK/CLS/MASK/SEP id mapping.
    tokenizer_files = (
        "vocab.json",
        "merges.txt",
        "tokenizer.json",
        "tokenizer_config.json",
        "special_tokens_map.json",
        "added_tokens.json",
    )
    for filename in tokenizer_files:
        source_file = Path(snapshot) / filename
        if source_file.is_file():
            shutil.copy2(source_file, out_dir / filename)

    sample_prompts = [
        "La radio grésille dans le sous-sol. Que remarques-tu ?",
        "Nous sommes devant une porte froide au laboratoire. Que proposes-tu ?",
        "Un ami a peur et veut rentrer chez lui. Que lui réponds-tu ?",
        "Une lumière clignote dans la forêt après minuit. Décris la scène.",
    ]
    samples = []
    for prompt in sample_prompts:
        samples.append({"prompt": prompt, "generated": generate_sample(model, tokenizer, prompt, str(device))})
    (out_dir / "samples_fr.jsonl").write_text(
        "".join(json.dumps(item, ensure_ascii=False) + "\n" for item in samples),
        encoding="utf-8")
    metrics = {
        "base_model": args.base,
        "base_revision": resolved_revision,
        "architecture": model.config.model_type,
        "parameter_count": parameter_count,
        "epochs": args.epochs,
        "batch_size": args.batch_size,
        "max_length": args.max_length,
        "learning_rate": args.learning_rate,
        "best_validation_token_loss": best_loss,
        "history": history,
        "elapsed_seconds": round(time.time() - start, 2),
        "train_examples": len(train_rows),
        "validation_examples": len(val_rows),
        "samples_file": "samples_fr.jsonl",
    }
    (out_dir / "metrics.json").write_text(
        json.dumps(metrics, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"[+] best val loss={best_loss:.4f}; checkpoint saved in {out_dir}", flush=True)


if __name__ == "__main__":
    main()
