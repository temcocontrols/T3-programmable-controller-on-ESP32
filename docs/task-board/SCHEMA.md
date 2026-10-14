# Existing NAS schema and attachment conventions

Original: reference/SCHEMA.md. Live task table: project.todos; primary key id bigint.

- Use exact project multiMeter; component is legacy.
- Maurice's lifecycle uses status, not the separate todo_status column.
- States: new, open, in progress, ready for qaqc, qaqc pass, qaqc fail, blocked, cancelled, superseded, resolved.
- Timestamps are Unix milliseconds in bigint; future updates bump updated_at.
- owner is the claimant; notes is append-only; qaqc_round matches rN.
- commit_sha, test_url and testing_evidence hold actual proof. Independent reviewer writes qaqc_score/qaqc_findings; pass is at least 9/10.
- Task 444 leaves optional table_view_id null. The CRM debug view was not verified or needed; no related rows were added.
- lijun's application role is user. SQL grants allow SELECT/INSERT/UPDATE on todos; sequence USAGE was verified. Wrapper protection is session read-only, not a dedicated server read-only role.
- The visible todos_doubao_guard trigger was inspected. Its extra rules apply to database user doubao, not lijun.

No schema/table/index creation, migrations, grants or trigger changes are allowed on the shared NAS. Original DDL is reference only.

