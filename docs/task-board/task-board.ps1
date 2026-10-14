param(
    [ValidateSet('Verify','Projects','List','Show')][string]$Mode = 'Verify',
    [long]$Id = 0,
    [switch]$PromptPassword
)
$ErrorActionPreference = 'Stop'
$cfg = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'config.json') -Raw | ConvertFrom-Json
$psql = (Get-Command psql -ErrorAction Stop).Source
$savedOptions = $env:PGOPTIONS
$savedTimeout = $env:PGCONNECT_TIMEOUT
$savedPassword = $env:PGPASSWORD
try {
    $credentialPath = Join-Path $env:LOCALAPPDATA 'Temco\TaskBoard\lijun.credential.xml'
    if (-not $PromptPassword -and -not $env:PGPASSWORD -and (Test-Path -LiteralPath $credentialPath)) {
        $credential = Import-Clixml -LiteralPath $credentialPath
        if ($credential -isnot [pscredential] -or $credential.UserName -cne $cfg.username) {
            throw 'Local credential does not match the configured database login.'
        }
        $env:PGPASSWORD = $credential.GetNetworkCredential().Password
    }
    # Enforce read-only sessions even when the login has UPDATE/INSERT privileges.
    $env:PGOPTIONS = '-c default_transaction_read_only=on -c statement_timeout=10000 -c lock_timeout=3000'
    $env:PGCONNECT_TIMEOUT = '5'
    $sql = switch ($Mode) {
        'Verify' { @'
SELECT current_user, current_database(), current_setting('transaction_read_only') AS read_only;
SELECT to_regclass('project.todos') AS task_table,
       has_table_privilege(current_user,'project.todos','SELECT') AS can_read;
SELECT username,role FROM project.users WHERE username=current_user;
SELECT project,count(*) FROM project.todos WHERE lower(project) LIKE '%meter%' GROUP BY project ORDER BY project;
'@ }
        'Projects' { 'SELECT project,count(*) FROM project.todos GROUP BY project ORDER BY project;' }
        'List' { @'
SELECT id,title,status,todo_status,priority,owner FROM project.todos
WHERE project=:'task_project' ORDER BY id DESC LIMIT 50;
'@ }
        'Show' { @'
SELECT id,title,description,status,todo_status,priority,owner,context,goal,
       acceptance_criteria,notes,blocked_reason,solution_summary,verification_summary,
       command,qaqc_round,qaqc_score,qaqc_findings,commit_sha,test_url,testing_evidence
FROM project.todos WHERE project=:'task_project' AND id=:'task_id'::bigint;
'@ }
    }
    if ($Mode -in @('List','Show')) {
        if ($cfg.project -cnotin @('Multimeter','multiMeter')) {
            throw 'Project is unverified. Confirm the exact existing database value before configuring project.'
        }
        if ($Mode -eq 'Show' -and $Id -le 0) { throw 'Show requires a positive task Id.' }
        $sql = @'
SELECT EXISTS(SELECT 1 FROM project.todos WHERE project=:'task_project') AS project_exists \gset
\if :project_exists
'@ + "`n" + $sql + "`n\else`n\echo 'Configured project is absent; no tasks read. Raising a read-only error to fail closed.'`nSELECT 1/0 AS project_not_found;`n\endif"
    }
    $auth = if ($PromptPassword) { '-W' } else { '-w' }
    $sql | & $psql -X $auth -h $cfg.host -p $cfg.port -U $cfg.username -d $cfg.database `
        -v ON_ERROR_STOP=1 -v "task_project=$($cfg.project)" -v "task_id=$Id" -P pager=off -f -
    if ($LASTEXITCODE -ne 0) { throw "Task-board read failed (psql exit $LASTEXITCODE)." }
} finally {
    $env:PGOPTIONS = $savedOptions
    $env:PGCONNECT_TIMEOUT = $savedTimeout
    $env:PGPASSWORD = $savedPassword
}
