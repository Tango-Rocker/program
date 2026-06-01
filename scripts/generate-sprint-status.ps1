param()

$repoRoot = Split-Path -Parent $PSScriptRoot
python "$repoRoot/tools/indexers/sprint_status.py"
