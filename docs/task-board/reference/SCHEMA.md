# Schema: project.todos

This matches a live Temco Controls table used by the CRM Debug tab (`table_view_id = 'debug'`).
Use it as a template. You can drop columns you do not need.

Timestamps are **Unix milliseconds** stored as `bigint` (not `timestamptz`).

## Core idea

| Role | How it shows up in the row |
|------|----------------------------|
| Which product / repo | `project` text |
| Who is working it | `owner` text (bot or person name) |
| Lifecycle | `status` text |
| Commit proof | `commit_sha` text |
| Round number | `qaqc_round` smallint |
| Review score | `qaqc_score` smallint (0–10), `qaqc_findings` text |

There is **no** separate `ready_for_qaqc` boolean. Readiness is the status value `'ready for qaqc'`.

## Status values (seen in production)

| status | meaning |
|--------|---------|
| `new` | Just created, not claimed |
| `open` | Available (default on INSERT) |
| `in progress` | Claimed; coding bot is working |
| `ready for qaqc` | Coding done; waiting for review |
| `qaqc pass` | Score ≥ 9/10 |
| `qaqc fail` | Score < 9; needs another round |
| `blocked` | Cannot proceed |
| `cancelled` | Dropped |
| `superseded` | Replaced by another row |
| `resolved` | Closed without full QAQC pass path |

Keep spelling exact (lowercase, spaces). Bots filter with `lower(status)`.

## Project column

- Use **`project`** to filter work (example: `'AndroidRide'`, `'CRM'`).
- An older column **`component`** still exists but is empty (legacy rename). Do not write new data there.
- Keep project names consistent. Mixed case (`CRM` vs `crm`) splits counts.

## Important columns

| column | type | default | notes |
|--------|------|---------|-------|
| id | bigint | sequence | Primary key. Use this as the row number in commits/tags. |
| todo_number | integer | null | Optional human number; unique when set |
| title | text | `''` | Short command / bug title |
| description | text | null | Longer body |
| status | text | `'open'` | See table above |
| created_at / updated_at | bigint | `0` | Unix ms; always bump `updated_at` on change |
| classification | varchar(50) | null | Seen: `command`, `regression` |
| table_view_id | text | null | CRM UI bucket; Debug uses `'debug'` |
| project | text | null | **Filter key** |
| notes | text | null | Append-only log (`[NEW]`, `[START]`, `[READY]`, …) |
| owner | text | null | Claim field (`cursor`, `doubao`, …) |
| source | text | null | Who created the row |
| created_by | text | `'system'` | Creator label |
| command | text | null | Full command text (optional) |
| commit_sha | text | null | Git SHA of the PRELIM commit |
| test_url | text | null | Path or URL to APK / build under test |
| qaqc_score | smallint | null | 0–10 |
| qaqc_findings | text | null | Reviewer notes |
| qaqc_round | smallint | `0` | Round N (matches `rN` in commit) |
| qaqc_at | bigint | null | When scored (Unix ms) |
| self_score | smallint | null | Optional coder self-score |
| blocked_reason | text | null | Why blocked |
| started_at / completed_at / resolved_at | bigint | null | Unix ms |
| acceptance_criteria | text | null | What “done” means |
| testing_evidence | text | null | Paths to proof |
| code_location | text | null | Files touched |
| parent_todo_id | integer | null | Parent row |
| related_todo_ids | text | null | Related ids (text list) |
| severity | text | null | Optional severity |
| screenshot_image / before_screenshot / after_screenshot | bytea | null | Optional inline images |
| component | text | null | **Legacy empty**; use `project` |
| cache_bust | text | null | Optional cache buster for installs |
| priority / category / goal / context / … | text | null | Optional planning fields |

Full column list (59):  
`id, todo_number, title, description, status, created_at, updated_at, number, classification, table_view_id, date, project, root_cause, notes, todo_status, context, goal, priority, category, subtasks, acceptance_criteria, implementation_notes, dependencies, related_bugs, related_docs, estimated_effort, blocked_reason, started_at, completed_at, resolved_at, what_went_wrong, solution_summary, verification_summary, prevention_notes, screenshot_image, before_screenshot, after_screenshot, playwright_screen_test_id, severity, user_complaint_count, false_claim_count, code_location, testing_evidence, last_verified_at, parent_todo_id, related_todo_ids, created_by, source, owner, command, commit_sha, cache_bust, test_url, qaqc_score, qaqc_findings, qaqc_round, qaqc_at, self_score, component`

## Indexes (production)

- PK on `id`
- Unique on `todo_number` where not null
- btree on `project`, `status`, `classification`, `table_view_id`, `component`, `parent_todo_id`, `playwright_screen_test_id`
- `(lower(status), updated_at DESC)`

## Related tables (optional)

You do not need these to start. They exist in the same schema:

### project.task_screenshots

`id, task_id, machine, agent, caption, created_at, file_path, sha256, thumbnail`

Stores screenshot metadata. `file_path` points at disk (example: `X:\CRM\Tasks\<id>\...`).

### project.fix_verification

Tracks fix attempts per todo (`todo_id`, `fix_attempt_number`, screenshots, `test_result`, …).

### project.table_views

UI view definitions (`id`, `name`, …). Debug view id is `'debug'`.

## CREATE TABLE (for your own Postgres)

Run as a DB user who can create schemas.

```sql
CREATE SCHEMA IF NOT EXISTS project;

CREATE SEQUENCE IF NOT EXISTS project.todos_id_seq;

CREATE TABLE IF NOT EXISTS project.todos (
  id                bigint PRIMARY KEY DEFAULT nextval('project.todos_id_seq'),
  todo_number       integer,
  title             text NOT NULL DEFAULT '',
  description       text,
  status            text DEFAULT 'open',
  created_at        bigint NOT NULL DEFAULT 0,
  updated_at        bigint NOT NULL DEFAULT 0,
  number            integer,
  classification    varchar(50),
  table_view_id     text,
  date              bigint,
  project           text,
  root_cause        text,
  notes             text,
  todo_status       text,
  context           text,
  goal              text,
  priority          text,
  category          text,
  subtasks          text,
  acceptance_criteria text,
  implementation_notes text,
  dependencies      text,
  related_bugs      text,
  related_docs      text,
  estimated_effort  text,
  blocked_reason    text,
  started_at        bigint,
  completed_at      bigint,
  resolved_at       bigint,
  what_went_wrong   text,
  solution_summary  text,
  verification_summary text,
  prevention_notes  text,
  screenshot_image  bytea,
  before_screenshot bytea,
  after_screenshot  bytea,
  playwright_screen_test_id integer,
  severity          text,
  user_complaint_count integer DEFAULT 0,
  false_claim_count integer DEFAULT 0,
  code_location     text,
  testing_evidence  text,
  last_verified_at  bigint,
  parent_todo_id    integer,
  related_todo_ids  text,
  created_by        text DEFAULT 'system',
  source            text,
  owner             text,
  command           text,
  commit_sha        text,
  cache_bust        text,
  test_url          text,
  qaqc_score        smallint,
  qaqc_findings     text,
  qaqc_round        smallint DEFAULT 0,
  qaqc_at           bigint,
  self_score        smallint,
  component         text  -- legacy; leave null; filter with project
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_todos_todo_number
  ON project.todos (todo_number) WHERE todo_number IS NOT NULL;
CREATE INDEX IF NOT EXISTS idx_todos_project ON project.todos (project);
CREATE INDEX IF NOT EXISTS idx_todos_status ON project.todos (status);
CREATE INDEX IF NOT EXISTS idx_todos_classification ON project.todos (classification);
CREATE INDEX IF NOT EXISTS idx_todos_table_view_id ON project.todos (table_view_id);
CREATE INDEX IF NOT EXISTS idx_todos_status_updated
  ON project.todos (lower(status), updated_at DESC);

-- Optional screenshots table
CREATE SEQUENCE IF NOT EXISTS project.task_screenshots_id_seq;
CREATE TABLE IF NOT EXISTS project.task_screenshots (
  id         bigint PRIMARY KEY DEFAULT nextval('project.task_screenshots_id_seq'),
  task_id    bigint NOT NULL REFERENCES project.todos(id),
  machine    text,
  agent      text,
  caption    text,
  created_at bigint NOT NULL,
  file_path  text NOT NULL,
  sha256     text NOT NULL,
  thumbnail  bytea
);
```

## Connect with psql

```bash
psql "postgresql://<user>:<password>@<host>:5432/<database>"
```

Or set `DATABASE_URL` in a local env file that is **not** committed.
