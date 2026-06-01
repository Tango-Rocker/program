$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ucrtBin = "C:\Users\Tango\tools\msys64\ucrt64\bin"
$cmakeExe = Join-Path $ucrtBin "cmake.exe"
$ninjaExe = Join-Path $ucrtBin "ninja.exe"
$gccExe = Join-Path $ucrtBin "gcc.exe"
$gxxExe = Join-Path $ucrtBin "g++.exe"
$buildDir = "cmake-build-debug"
$targets = @("sim_tests", "sim_headless")

if (-not (Test-Path $cmakeExe)) {
    throw "cmake not found: $cmakeExe"
}
if (-not (Test-Path $ninjaExe)) {
    throw "ninja not found: $ninjaExe"
}
if (-not (Test-Path $gccExe)) {
    throw "gcc not found: $gccExe"
}
if (-not (Test-Path $gxxExe)) {
    throw "g++ not found: $gxxExe"
}

$env:PATH = "$ucrtBin;$env:PATH"

$configureArgs = @(
    "--fresh"
    "-S"
    "."
    "-B"
    $buildDir
    "-G"
    "Ninja"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    "-DCMAKE_C_COMPILER:FILEPATH=$gccExe"
    "-DCMAKE_CXX_COMPILER:FILEPATH=$gxxExe"
    "-DCMAKE_MAKE_PROGRAM:FILEPATH=$ninjaExe"
)

& $cmakeExe @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $cmakeExe --build $buildDir --target @targets
exit $LASTEXITCODE
