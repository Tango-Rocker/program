param()

$repoRoot = Split-Path -Parent $PSScriptRoot
python "$repoRoot/tools/indexers/module_index.py"
python "$repoRoot/tools/indexers/sprint_status.py"
