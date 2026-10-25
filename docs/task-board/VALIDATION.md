# Validation — 2026-10-14

Updated after the user supplied Maurice's five original files and authorized the first real task.
- Original documents copied unchanged to reference/ and SHA256 verified against supplied files.
- Confirmed existing NAS table, login and sequence USAGE. No schema changes.
- Inspected the INSERT/UPDATE trigger; additional restrictions apply only to doubao.
- One authorized task inserted in a transaction with a cooperative advisory lock and duplicate check: 444, exact project multiMeter, English task summary: Debug the new hardware revision and test voltage measurement, status new, owner null. The database title retains the original user-provided wording; this documentation change does not update the shared database.
- No acceptance thresholds invented; description/command repeat the user's task. No debug view assumed.
- config.json now uses the database-confirmed multiMeter value.
- Windows DPAPI-encrypted credential created outside Git at %LOCALAPPDATA%\Temco\TaskBoard\lijun.credential.xml. No plaintext credential stored in project files.
- Wrapper remains read-only, with timeouts/project/id guards and environment restoration.
- Null-project and absent-project guards were tested before insertion. Verify/List/Show passed after insertion using the encrypted local credential without a password prompt. Verify returned read_only=on; List/Show returned task 444 as new with empty owner. Environment restoration passed. Windows PowerShell 5.1 List also passed.
- Existing four tracked modifications preserved. No firmware edits, build, flashing, task claim, commit or push.
- Original status conventions reconciled: status authoritative, Unix-ms timestamps, append-only notes, independent QAQC >=9.
- Copilot UI capture repeatedly timed out, so Agent selection/sign-in and instruction loading could not be verified. A reusable prompt was prepared and requested open in the existing VS Code window. The extension listing does not show a separate Copilot extension, though a Copilot runtime process was observed earlier. User must complete any account/approval prompt.


