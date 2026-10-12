param([switch]$Live)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools are required.' }
$out = Join-Path $root 'artifacts\tests'
New-Item -ItemType Directory -Force $out | Out-Null
$command = 'call "{0}\VC\Auxiliary\Build\vcvars64.bat" >nul && cl /nologo /EHsc /std:c++20 /W4 /utf-8 /DCODEX_TELEMETRY_TEST /Fo"{1}\\" /Fe"{1}\codex-tests.exe" "{2}\tests\codex_telemetry_tests.cpp" "{2}\TrafficMonitor\CodexTelemetry.cpp" && "{1}\codex-tests.exe" {3}' -f $vs, $out, $root, $(if ($Live) {'--live'} else {''})
& $env:ComSpec /d /c $command
if ($LASTEXITCODE) { throw "Codex telemetry checks failed ($LASTEXITCODE)." }
