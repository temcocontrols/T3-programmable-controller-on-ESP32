# Queries (copy-paste)

Replace placeholders:

- `<project>` → your project name (example: `AndroidRide`)
- `<bot>` → `cursor` / `vscode` / `doubao` / your name
- `<id>` → row id
- Times: Unix milliseconds. In SQL you can use `(EXTRACT(EPOCH FROM now()) * 1000)::bigint`.

Connect:

```bash
psql "postgresql://<user>:<password>@<host>:5432/<database>"
```

---

## Filter by project

```sql
SELECT id, status, owner, qaqc_round, qaqc_score,
       LEFT(title, 80) AS title, commit_sha, updated_at
FROM project.todos
WHERE project = '<project>'
ORDER BY id DESC;
```

## Counts by status for one project

```sql
SELECT status, COUNT(*) AS n
FROM project.todos
WHERE project = '<project>'
GROUP BY status
ORDER BY n DESC;
```

## List distinct projects

```sql
SELECT COALESCE(project, '(null)') AS project, COUNT(*) AS n
FROM project.todos
GROUP BY 1
ORDER BY n DESC;
```

## My open rows (I own them, not finished)

```sql
SELECT id, status, qaqc_round, LEFT(title, 80) AS title, commit_sha
FROM project.todos
WHERE project = '<project>'
  AND owner = '<bot>'
  AND lower(status) IN ('in progress', 'qaqc fail', 'ready for qaqc')
ORDER BY updated_at DESC;
```

## Claim next free row (safe pattern)

Run in a transaction. Only one bot should win.

```sql
BEGIN;

WITH next_row AS (
  SELECT id
  FROM project.todos
  WHERE project = '<project>'
    AND (owner IS NULL OR btrim(owner) = '')
    AND lower(status) IN ('new', 'open')
  ORDER BY id
  FOR UPDATE SKIP LOCKED
  LIMIT 1
)
UPDATE project.todos t
SET owner      = '<bot>',
    status     = 'in progress',
    started_at = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    updated_at = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    qaqc_round = CASE WHEN COALESCE(t.qaqc_round, 0) < 1 THEN 1 ELSE t.qaqc_round END,
    notes      = COALESCE(t.notes, '') || E'\n[START] owner=<bot>'
FROM next_row n
WHERE t.id = n.id
RETURNING t.id, t.title, t.status, t.owner, t.qaqc_round;

COMMIT;
```

### Claim a specific id

```sql
UPDATE project.todos
SET owner      = '<bot>',
    status     = 'in progress',
    started_at = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    updated_at = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    qaqc_round = CASE WHEN COALESCE(qaqc_round, 0) < 1 THEN 1 ELSE qaqc_round END,
    notes      = COALESCE(notes, '') || E'\n[START] owner=<bot>'
WHERE id = <id>
  AND project = '<project>'
  AND (owner IS NULL OR btrim(owner) = '' OR owner = '<bot>')
  AND lower(status) IN ('new', 'open', 'qaqc fail')
RETURNING id, status, owner, qaqc_round;
```

## Mark ready for QAQC

```sql
UPDATE project.todos
SET status     = 'ready for qaqc',
    commit_sha = '<sha>',
    test_url   = '<path-or-url>',
    updated_at = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    notes      = COALESCE(notes, '') || E'\n[READY] round ' || COALESCE(qaqc_round, 1)::text
                 || ' commit <sha> url <path-or-url>'
WHERE id = <id>
  AND owner = '<bot>'
RETURNING id, status, qaqc_round, commit_sha, test_url;
```

## Rows waiting for QAQC

```sql
SELECT id, project, owner, qaqc_round, commit_sha, test_url,
       LEFT(title, 80) AS title
FROM project.todos
WHERE lower(status) = 'ready for qaqc'
  AND (<project> IS NULL OR project = '<project>')  -- or drop this line and filter in app
ORDER BY updated_at;
```

Simpler:

```sql
SELECT id, project, owner, qaqc_round, commit_sha, test_url, LEFT(title, 80) AS title
FROM project.todos
WHERE project = '<project>'
  AND lower(status) = 'ready for qaqc'
ORDER BY updated_at;
```

## Record a QAQC score (pass)

Pass = 9 or 10.

```sql
UPDATE project.todos
SET status        = 'qaqc pass',
    qaqc_score    = 9,
    qaqc_findings = '<short findings>',
    qaqc_at       = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    updated_at    = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    completed_at  = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    notes         = COALESCE(notes, '') || E'\n[PASS] score 9'
WHERE id = <id>
RETURNING id, status, qaqc_score, qaqc_round;
```

Then in git:

```bash
git tag pass-<id>-rN
git push origin pass-<id>-rN
```

## Record a QAQC score (fail)

```sql
UPDATE project.todos
SET status        = 'qaqc fail',
    qaqc_score    = 6,
    qaqc_findings = '<exact fixes required>',
    qaqc_at       = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    updated_at    = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    notes         = COALESCE(notes, '') || E'\n[QAQC FAIL] score 6: <exact fixes required>'
WHERE id = <id>
RETURNING id, status, qaqc_score, qaqc_findings;
```

## Start next round after fail

```sql
UPDATE project.todos
SET status     = 'in progress',
    owner      = '<bot>',
    qaqc_round = COALESCE(qaqc_round, 0) + 1,
    updated_at = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    notes      = COALESCE(notes, '') || E'\n[START] round ' || (COALESCE(qaqc_round, 0) + 1)::text
WHERE id = <id>
  AND lower(status) = 'qaqc fail'
RETURNING id, status, qaqc_round, owner;
```

## Add a new project row (command)

```sql
INSERT INTO project.todos (
  title, description, status, project, classification, table_view_id,
  source, created_by, created_at, updated_at, notes, command
) VALUES (
  '<short title>',
  '<full instructions>',
  'new',
  '<project>',
  'command',
  'debug',
  '<your-name>',
  '<your-name>',
  (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
  (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
  '[NEW] project=<project>',
  '<full instructions>'
)
RETURNING id, project, status, title;
```

## Block a row

```sql
UPDATE project.todos
SET status         = 'blocked',
    blocked_reason = '<why>',
    updated_at     = (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
    notes          = COALESCE(notes, '') || E'\n[BLOCKED] <why>'
WHERE id = <id>
RETURNING id, status, blocked_reason;
```

## Insert a screenshot metadata row (optional)

```sql
INSERT INTO project.task_screenshots (
  task_id, machine, agent, caption, created_at, file_path, sha256
) VALUES (
  <id>,
  '<machine>',
  '<bot-or-tester>',
  '<what this shows>',
  (EXTRACT(EPOCH FROM now()) * 1000)::bigint,
  '<full path to png>',
  '<sha256 of file>'
)
RETURNING id, task_id, file_path;
```
