# Postgres task table (for coding bots)

These docs explain how coding bots and a QAQC reviewer share work through one Postgres table.

Copy this folder to your own machine. Point the SQL at your own Postgres. Use your own project names.

## Who this is for

- You run one or more coding bots (Cursor, VS Code Copilot, Doubao, etc.).
- You want them to pick tasks, commit, and hand work to a reviewer without chat side channels.
- All status, scores, and findings live in the row.

## Files in this folder

| File | What it covers |
|------|----------------|
| [SCHEMA.md](SCHEMA.md) | Real columns, status values, CREATE TABLE DDL |
| [WORKFLOW.md](WORKFLOW.md) | Row lifecycle, commits, tags, QAQC |
| [QUERIES.md](QUERIES.md) | Copy-paste SQL for daily use |
| [BOT-PROMPT.md](BOT-PROMPT.md) | Ready-to-paste instructions for a coding bot |

## Quick start

1. Create schema + table (see SCHEMA.md).
2. Insert a row with `project = 'YourProject'` and `status = 'new'` or `'open'`.
3. Paste BOT-PROMPT.md into your coding bot. Set the project name.
4. Bot claims the row (`owner` + `in progress`), works, commits `[PRELIM] <id> rN`, sets `ready for qaqc`.
5. Helper tests the build. Screenshots go under `X:\CRM\Tasks\<id>\<tester>\` (or your own Tasks folder).
6. Reviewer scores 0–10 into `qaqc_score` / `qaqc_findings`. Pass = 9/10 → tag `pass-<id>-rN` and status `qaqc pass`.

## Connection (placeholders only)

```text
postgresql://<user>:<password>@<host>:5432/<database>
```

Never put real passwords in docs, chat, or git.

## Important rules

- Filter by `project` (example: `project = 'AndroidRide'`).
- Communication between bots and QAQC goes **only through the row** (status, notes, score, findings). No side channels.
- Two bots must not edit the same file at the same time. Split ownership (example: one bot owns MainActivity + build file).
- One handoff file per project folder.
- Scratch files only in a scratch folder (example: `C:\temp\agent-scratch`).
- Push to a working branch. No pull requests unless someone asks.
