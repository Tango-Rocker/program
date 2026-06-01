$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ucrtBin = "C:\Users\Tango\tools\msys64\ucrt64\bin"
$serverCmd = "C:\Users\Tango\AppData\Roaming\npm\lsp-mcp-server.cmd"
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path $ucrtBin)) {
    throw "UCRT bin directory not found: $ucrtBin"
}
if (-not (Test-Path $serverCmd)) {
    throw "lsp-mcp-server not found: $serverCmd"
}
if (-not (Test-Path (Join-Path $repoRoot ".lsp-mcp.json"))) {
    throw "LSP MCP config not found in repo root: $repoRoot"
}

$env:PATH = "$ucrtBin;$env:PATH"
Set-Location $repoRoot
& $serverCmd @args
exit $LASTEXITCODE
