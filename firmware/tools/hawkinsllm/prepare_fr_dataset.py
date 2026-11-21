#!/usr/bin/env python3
"""Prepare a deterministic, French-only fine-tuning corpus for Hawkins TinyTalk."""

from __future__ import annotations
import argparse
import hashlib
import json
import re
from pathlib import Path
from typing import Any

URL_RE = re.compile(r"https?://\S+|www\.\S+", re.IGNORECASE)
EMAIL_RE = re.compile(r"\b[\w.+-]+@[\w.-]+\.[A-Za-z]{2,}\b")
PHONE_RE = re.compile(r"(?<!\w)(?:\+?\d[\d .()/-]{7,}\d)(?!\w)")
EOS = "<|endoftext|>"
UNSAFE_LABELS = {"pii", "not_appropriate", "hate_speech", "sexual_content", "spam", "lang_mismatch", "fails_task"}


def clean_text(value: Any) -> str:
    text = str(value or "").replace(EOS, " ")
    text = URL_RE.sub(" ", text)
    text = EMAIL_RE.sub(" ", text)
    text = PHONE_RE.sub(" ", text)
    text = re.sub(r"[\r\n\t]+", " ", text)
    text = re.sub(r"\s+", " ", text).strip()
    return text


def hash_bucket(value: str, buckets: int = 10) -> int:
    digest = hashlib.sha256(value.encode("utf-8")).digest()
    return int.from_bytes(digest[:4], "big") % buckets


def unsafe_labels(row: dict[str, Any]) -> bool:
    labels = row.get("labels")
    if isinstance(labels, dict):
        names = labels.get("name") or labels.get("names") or []
        values = labels.get("value") or labels.get("values") or []
        if isinstance(names, list) and isinstance(values, list):
            for name, value in zip(names, values):
                try:
                    if str(name).lower() in UNSAFE_LABELS and float(value) >= 0.5:
                        return True
                except (TypeError, ValueError):
                    continue
    detox = row.get("detoxify")
    if isinstance(detox, dict):
        for key in ("severe_toxicity", "identity_attack", "threat", "sexual_explicit"):
            try:
                if float(detox.get(key) or 0.0) >= 0.25:
                    return True
            except (TypeError, ValueError):
                continue
        try:
            if float(detox.get("toxicity") or 0.0) >= 0.70:
                return True
        except (TypeError, ValueError):
            pass
    return False


def valid_prompt(row: dict[str, Any]) -> bool:
    return (
        str(row.get("role", "")).lower() == "prompter"
        and str(row.get("lang", "")).lower() == "fr"
        and not bool(row.get("deleted", False))
    )


def valid_answer(row: dict[str, Any]) -> bool:
    if str(row.get("role", "")).lower() != "assistant":
        return False
    if str(row.get("lang", "")).lower() != "fr":
        return False
    if bool(row.get("deleted", False)) or bool(row.get("synthetic", False)):
        return False
    if row.get("review_result") is not True or unsafe_labels(row):
        return False
    rank = row.get("rank")
    if rank not in (None, 0):
        return False
    return True


def load_seeds(path: Path) -> list[dict[str, str]]:
    rows = []
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError as exc:
            raise SystemExit(f"seed_fr.jsonl:{line_no}: JSON invalide: {exc}") from exc
        prompt, response = clean_text(row.get("prompt")), clean_text(row.get("response"))
        if len(prompt) < 8 or len(response) < 8:
            raise SystemExit(f"seed_fr.jsonl:{line_no}: prompt/réponse trop courts")
        rows.append({"id": str(row.get("id") or f"seed-{line_no}"),
                     "prompt": prompt, "response": response, "source": "hawkins-original"})
    if len(rows) < 20:
        raise SystemExit("Il faut au moins 20 dialogues originaux Hawkins dans seed_fr.jsonl")
    return rows


def load_oasst(max_pairs: int) -> tuple[list[dict[str, str]], dict[str, Any]]:
    from datasets import load_dataset

    print("[+] Téléchargement du jeu public OpenAssistant/oasst1 (split train)", flush=True)
    dataset = load_dataset("OpenAssistant/oasst1", split="train")
    by_id: dict[str, dict[str, Any]] = {}
    for row in dataset:
        message_id = row.get("message_id")
        if message_id:
            by_id[str(message_id)] = dict(row)

    pairs: list[dict[str, str]] = []
    seen: set[tuple[str, str]] = set()
    for answer in by_id.values():
        if not valid_answer(answer):
            continue
        parent_id = answer.get("parent_id")
        prompt_row = by_id.get(str(parent_id)) if parent_id else None
        if not prompt_row or not valid_prompt(prompt_row):
            continue
        prompt = clean_text(prompt_row.get("text"))
        response = clean_text(answer.get("text"))
        if not (12 <= len(prompt) <= 260 and 12 <= len(response) <= 320):
            continue
        if len(prompt.split()) > 42 or len(response.split()) > 58:
            continue
        key = (prompt.casefold(), response.casefold())
        if key in seen:
            continue
        seen.add(key)
        pairs.append({
            "id": str(answer.get("message_id")),
            "prompt": prompt,
            "response": response,
            "source": "OpenAssistant/oasst1-fr",
        })

    pairs.sort(key=lambda row: hashlib.sha256(row["id"].encode("utf-8")).hexdigest())
    pairs = pairs[:max_pairs]
    info = {
        "dataset": "OpenAssistant/oasst1",
        "split": "train",
        "license": "Apache-2.0",
        "language_filter": "fr",
        "accepted_pairs": len(pairs),
        "max_pairs": max_pairs,
    }
    print(f"[+] Paires françaises OASST1 retenues : {len(pairs)}", flush=True)
    return pairs, info


def write_jsonl(path: Path, rows: list[dict[str, str]]) -> None:
    path.write_text("".join(json.dumps(row, ensure_ascii=False) + "\n" for row in rows),
                    encoding="utf-8")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--output-dir", required=True)
    ap.add_argument("--max-oasst1", type=int, default=1200)
    ap.add_argument("--seed-repeat", type=int, default=4)
    ap.add_argument("--seed", type=int, default=1703)
    args = ap.parse_args()
    if args.max_oasst1 < 100 or args.seed_repeat < 1:
        raise SystemExit("--max-oasst1 doit être >= 100 et --seed-repeat >= 1")

    root = Path(__file__).resolve().parent
    output = Path(args.output_dir)
    output.mkdir(parents=True, exist_ok=True)

    oasst, source_info = load_oasst(args.max_oasst1)
    seeds = load_seeds(root / "seed_fr.jsonl")
    if len(oasst) < 80:
        raise SystemExit(
            f"Seulement {len(oasst)} paires OASST1 françaises de qualité ont été trouvées; "
            "arrêt pour éviter de fabriquer un modèle prétendument français avec trop peu de données."
        )

    train_oasst, val_oasst = [], []
    for row in oasst:
        (val_oasst if hash_bucket(row["id"]) == 0 else train_oasst).append(row)

    train_seeds, val_seeds = [], []
    for index, row in enumerate(seeds):
        (val_seeds if index % 5 == 0 else train_seeds).append(row)

    # Les exemples Hawkins originaux sont répétés modérément pour orienter le ton,
    # sans remplacer le corpus français général ni contaminer la validation.
    train = train_oasst + train_seeds * args.seed_repeat
    validation = val_oasst + val_seeds
    if len(validation) < 12:
        raise SystemExit("Jeu de validation trop petit; au moins 12 exemples sont requis.")

    import random
    random.Random(args.seed).shuffle(train)
    random.Random(args.seed + 1).shuffle(validation)

    write_jsonl(output / "train.jsonl", train)
    write_jsonl(output / "validation.jsonl", validation)

    all_rows = train_oasst + val_oasst + train_seeds + val_seeds
    corpus = []
    for row in all_rows:
        corpus.append(f"User: {row['prompt']}\nBot: {row['response']}{EOS}\n")
    (output / "corpus.txt").write_text("\n".join(corpus), encoding="utf-8")

    manifest = {
        "seed": args.seed,
        "base_model": "TheREZOR/TinyTalk",
        "base_model_license": "CC-BY-4.0",
        "corpus_license": "Apache-2.0 for OASST1; project-authored Hawkins seeds",
        "oasst1": source_info,
        "hawkins_seed_count": len(seeds),
        "hawkins_seed_repeat": args.seed_repeat,
        "train_examples_after_oversampling": len(train),
        "validation_examples": len(validation),
        "context_target_tokens": 80,
        "privacy_filters": ["URLs", "email addresses", "phone numbers", "OASST1 PII/toxicity labels"],
    }
    (output / "dataset_manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"[+] corpus prêt : train={len(train)}, validation={len(validation)}, "
          f"corpus vocab={len(all_rows)} exemples", flush=True)


if __name__ == "__main__":
    main()
