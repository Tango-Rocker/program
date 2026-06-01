$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildScript = Join-Path $PSScriptRoot "build-debug.ps1"

if (-not (Test-Path $buildScript)) {
    throw "build script not found: $buildScript"
}

Push-Location $repoRoot
try {
    & $buildScript
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    if (Test-Path (Join-Path $repoRoot ".git")) {
        & git diff --check
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }

        & git diff --cached --check
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
} finally {
    Pop-Location
}
