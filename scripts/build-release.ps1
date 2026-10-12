param([string]$Version = '1.87.0')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Only stable three-part versions are allowed.' }
if (git status --porcelain) { throw 'Commit changes before building a release.' }
$commit = (git rev-parse HEAD).Trim()
$sourceVersion = [regex]::Match([IO.File]::ReadAllText((Join-Path $root 'TrafficMonitor\stdafx.h')), '#define VERSION L"([^"]+)"').Groups[1].Value
if ($sourceVersion -ne $Version) { throw 'Source version does not match the requested release.' }
$output = Join-Path $root "artifacts\release-v$Version-$($commit.Substring(0,12))"
if (Test-Path (Join-Path $output 'SHA256SUMS.txt')) { throw 'This release was already packaged. Reuse the verified output; do not rebuild it.' }
New-Item -ItemType Directory -Force $output | Out-Null
$source = Join-Path $output 'source'
if (Test-Path $source) { throw 'A previous build exists here. Preserve it and use a fresh version or resolve the failed build explicitly.' }
$archive = Join-Path $output 'source.zip'
git archive --format=zip "--output=$archive" HEAD
if ($LASTEXITCODE) { throw 'git archive failed.' }
Expand-Archive -LiteralPath $archive -DestinationPath $source
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
if (!$vs) { throw 'Visual Studio 2022 with C++ and MFC is required. Source artifacts were preserved.' }
$msbuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
& (Join-Path $source 'scripts\test-codex.ps1')
if ($LASTEXITCODE) { throw 'Tests failed.' }
$runtime = Get-ChildItem (Join-Path $vs 'VC\Redist\MSVC') -Directory | Where-Object Name -Match '^\d+\.\d+\.\d+$' | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$runtime) { throw 'Visual C++ redistributable directory missing.' }
$files = @()
foreach ($arch in @('x64','x86')) {
    $log = Join-Path $output "build-$arch.log"
    & $msbuild (Join-Path $source 'TrafficMonitor_Lite.sln') /t:TrafficMonitor /m /nologo '/p:Configuration=Release (lite)' "/p:Platform=$arch" /p:PlatformToolset=v143 /v:minimal *> $log
    if ($LASTEXITCODE) { throw "Build failed for $arch. See $log. Source and logs were preserved." }
    $bin = if ($arch -eq 'x64') { Join-Path $source 'Bin\x64\Release (lite)' } else { Join-Path $source 'Bin\Release (lite)' }
    $exe = Join-Path $bin 'TrafficMonitor.exe'
    if (!(Test-Path $exe) -or [Diagnostics.FileVersionInfo]::GetVersionInfo($exe).FileVersion -ne "$Version.0") { throw "Actual $arch executable version mismatch." }
    $payload = Join-Path $output "portable-$arch"
    New-Item -ItemType Directory -Path $payload | Out-Null
    Copy-Item -LiteralPath $exe -Destination $payload
    foreach ($directory in @('Microsoft.VC143.CRT','Microsoft.VC143.MFC')) {
        $dllPath = Join-Path $runtime.FullName "$arch\$directory"
        if (!(Test-Path $dllPath)) { throw "Required runtime directory missing: $directory ($arch)." }
        Get-ChildItem -LiteralPath $dllPath -Filter *.dll -File | Copy-Item -Destination $payload
    }
    Copy-Item -LiteralPath (Join-Path $source 'TrafficMonitor\skins') -Destination $payload -Recurse
    Copy-Item -LiteralPath (Join-Path $source 'TrafficMonitor\language') -Destination $payload -Recurse
    Copy-Item -LiteralPath (Join-Path $source 'LICENSE'), (Join-Path $source 'LICENSE_CN') -Destination $payload
    Copy-Item -LiteralPath (Join-Path $source 'third_party\nlohmann\LICENSE.MIT') -Destination (Join-Path $payload 'nlohmann-json-LICENSE.MIT')
    Copy-Item -LiteralPath (Join-Path $source 'docs\README-release.txt') -Destination (Join-Path $payload 'README.txt')
    if (!(Select-String -LiteralPath (Join-Path $source 'TrafficMonitor\TrafficMonitor.cpp') -SimpleMatch 'm_general_data.update_source = 0;')) { throw 'GitHub update default was not verified.' }
    $forbidden = Get-ChildItem -LiteralPath $payload -Recurse -File | Where-Object { $_.Name -in @('auth.json','config.ini','update-share.json','version.info','version_utf8.info') }
    if ($forbidden) { throw 'Unexpected configuration in portable payload.' }
    $zip = Join-Path $output "TrafficMonitor-v$Version-$arch.zip"
    Compress-Archive -Path (Join-Path $payload '*') -DestinationPath $zip
    $verify = Join-Path $output "verify-$arch"
    Expand-Archive -LiteralPath $zip -DestinationPath $verify
    if ((Get-FileHash (Join-Path $verify 'TrafficMonitor.exe')).Hash -ne (Get-FileHash $exe).Hash) { throw 'Portable ZIP executable hash mismatch.' }
    $expected = @(Get-ChildItem $payload -Recurse -File)
    $actual = @(Get-ChildItem $verify -Recurse -File)
    if ($expected.Count -ne $actual.Count) { throw 'Portable ZIP file count mismatch.' }
    foreach ($file in $expected) {
        $relative = [IO.Path]::GetRelativePath($payload, $file.FullName)
        if ((Get-FileHash $file.FullName).Hash -ne (Get-FileHash (Join-Path $verify $relative)).Hash) { throw "ZIP payload mismatch: $relative" }
    }
    $files += $zip
}
Copy-Item -LiteralPath (Join-Path $source 'docs\README-release.txt') -Destination (Join-Path $output 'README.txt')
$files += Join-Path $output 'README.txt'
$sums = $files | ForEach-Object { (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant() + '  ' + [IO.Path]::GetFileName($_) }
[IO.File]::WriteAllLines((Join-Path $output 'SHA256SUMS.txt'), $sums, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $output 'commit.txt'), $commit, [Text.UTF8Encoding]::new($false))
Write-Output "Verified release packages: $output"
