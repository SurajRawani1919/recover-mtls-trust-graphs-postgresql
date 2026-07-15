# Experts Platform — Rubric & Submission Steps

Use this after local validation passes (`./scripts/run-harness.sh verify`).

---

## Step 1: Fix submission access (required)

Submission fails with:

```
Forbidden: User does not have access to this project
```

Your API key works for `stb keys refresh`, but **`stb projects list` returns empty** — the account is not enrolled as a submitter.

**Fix (5 minutes):**

1. Open the **Snorkel Experts platform** in your browser
2. Confirm you are assigned to **Terminus-2nd-Edition** (submitter role)
3. If not listed, contact your project lead / Snorkel coordinator to add you
4. In WSL, re-authenticate with that account:

```bash
wsl -d Ubuntu-24.04
export PATH=$HOME/.local/bin:/usr/bin:/bin
stb login
```

5. Verify access:

```bash
stb projects list
```

You should see **Terminus-2nd-Edition** (not "No projects found").

6. Update `scripts/load-stb-env.sh` with the same API key if you use that file.

---

## Step 2: Submit via CLI

```bash
cd /mnt/c/Users/rawan/OneDrive/Desktop/recover-mtls-trust-graphs-postgresql
source scripts/load-stb-env.sh
./scripts/run-harness.sh submit Terminus-2nd-Edition 180
```

This removes `jobs/` and `my-task-name/` before zipping (~107 KB, not 2.5 MB).

On success you will see:

- Upload to S3
- `Running checks...` (CI / LLMaJ — automated platform validation)
- `Feedback completed with outcome: PASS`
- Submission created

---

## Step 3: Paste rubric on Experts platform (UI)

Open your submission in the Experts platform and copy fields from **`RUBRIC.md`**.

Quick reference:

| Field | Value |
|-------|-------|
| Task name | `snorkel/recover-mtls-trust-graphs-postgresql` |
| Difficulty | Hard |
| Domain | Security & Cryptography |
| Subtypes | API Integration, Tool Specific |
| Handling time | 180 minutes |
| Oracle reward | 1.0 (8/8 tests) |

Paste the **Rubric criteria** section (6 categories) from `RUBRIC.md` into the platform form.

---

## Local validation status (already done)

| Check | Result |
|-------|--------|
| Docker Compose in WSL | ✅ 2.40.3 |
| Oracle (`stb harbor run -a oracle`) | ✅ Mean 1.000 |
| GPT-5.5 agent run | ✅ Mean 1.000 |
| Claude Opus 4.8 agent run | ✅ Mean 1.000 |
| Submission zip (dry-run) | ✅ ~107 KB |
| Platform project access | ❌ Needs enrollment |
| Rubric paste | ⏳ After submission |
| CI / LLMaJ | ⏳ Runs automatically on upload |

---

## Difficulty note

Both GPT-5.5 and Claude passed on first trial. For a **hard** task, consider running 2–3 more trials or increasing difficulty before final reviewer submission. See `RUBRIC.md` calibration section.
