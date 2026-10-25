# multiMeter task board

Configured on 2026-10-14. Workspace: C:\Users\Administrator\Documents\GitHub\MultiSensor.
Active source: main/multiMeter.c; another implementation lives in STM32/Multimeter/.

## Database and first task

NAS 192.168.0.3:5432; database postgres; login lijun. Exact project: multiMeter.
Task **444 — Debug the new hardware revision and test voltage measurement** was created with user authorization, status new, owner null.
No task claim, firmware changes or hardware tests were performed.

Maurice's originals are in reference/ (copied unchanged and hash-verified). The top-level documents adapt them to the existing NAS. NEVER execute the original bootstrap CREATE SCHEMA/TABLE/INDEX commands here.

## Commands from the repository root

```powershell
& .\docs\task-board\task-board.ps1 -Mode Verify
& .\docs\task-board\task-board.ps1 -Mode List
& .\docs\task-board\task-board.ps1 -Mode Show -Id 444
```

Authentication uses a Windows DPAPI-encrypted credential outside Git:
%LOCALAPPDATA%\Temco\TaskBoard\lijun.credential.xml.
Only the same Windows user on this machine can normally decrypt it. Do not upload it, inspect its contents in chat, or print decrypted credentials. The wrapper temporarily passes the password through the psql child environment and restores the previous environment afterwards. Processes under this Windows account are trusted with database access.
For a different user/machine use -PromptPassword interactively or arrange local credentials there. Never send the password to a bot or store plaintext in code/docs/Git.

The wrapper is read-only: Verify, Projects, List, Show. Project/id guards apply to task content. No reusable write tool is installed. Adding task 444 was a separate, one-time authorized transaction.

## Copilot

Instructions: .github/instructions/multimeter-task-board.instructions.md.
Reusable Agent prompt: .github/prompts/multimeter-task-board.prompt.md.
In Copilot Chat invoke /multimeter-task-board, or paste the local BOT-PROMPT.md instructions.
Sign-in and any specific command approval belong to the user if requested. Do not enable blanket approval.

Node.js 18.15.0 and psql 18.6 are installed; the proposed WooCommerce CLI path is absent and unverified.
Further task writes/claims, firmware edits, build, flashing, commits and pushes need separate authorization.

