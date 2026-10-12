param([string]$Version = '1.87.0', [string]$NotesFile = 'docs\releases\v1.87.0.md', [string]$ArtifactDirectory)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
. (Join-Path $PSScriptRoot 'github-auth.ps1')
if (git status --porcelain) { throw 'The workspace must be clean before publishing.' }
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Stable versions only.' }
$commit = (git rev-parse HEAD).Trim()
$branch = (git branch --show-current).Trim()
if (!$branch -and $env:GITHUB_REF_TYPE -eq 'branch') { $branch = $env:GITHUB_REF_NAME }
if (!$branch) { throw 'Release publishing requires a branch checkout.' }
$remote = git ls-remote origin "refs/heads/$branch"
if ($LASTEXITCODE -or !$remote -or ($remote -split '\s+')[0] -ne $commit) { throw 'Push the current commit to GitHub before publishing.' }
if (!$ArtifactDirectory) { $ArtifactDirectory = Join-Path $root "artifacts\release-v$Version-$($commit.Substring(0,12))" }
$ArtifactDirectory = [IO.Path]::GetFullPath($ArtifactDirectory)
$required = @("TrafficMonitor-v$Version-x64.zip", "TrafficMonitor-v$Version-x86.zip", 'SHA256SUMS.txt', 'README.txt')
foreach ($name in $required + @('commit.txt')) { if (!(Test-Path (Join-Path $ArtifactDirectory $name))) { throw "Missing release file: $name" } }
if ([IO.File]::ReadAllText((Join-Path $ArtifactDirectory 'commit.txt')).Trim() -ne $commit) { throw 'Artifacts belong to a different commit.' }
$verifiedNames = @()
foreach ($line in [IO.File]::ReadAllLines((Join-Path $ArtifactDirectory 'SHA256SUMS.txt'))) {
    if ($line -notmatch '^([a-f0-9]{64})  ([^/\\]+)$') { throw 'Invalid checksum manifest.' }
    if ($Matches[2] -notin $required -or (Get-FileHash (Join-Path $ArtifactDirectory $Matches[2])).Hash.ToLowerInvariant() -ne $Matches[1]) { throw 'Local checksum mismatch.' }
    $verifiedNames += $Matches[2]
}
if (@(Compare-Object ($required | Where-Object { $_ -ne 'SHA256SUMS.txt' }) $verifiedNames).Count) { throw 'Checksum manifest is incomplete or contains duplicates.' }
$headers = Get-GitHubHeaders
$api = 'https://api.github.com/repos/Francis0927/TrafficMonitor'
$tag = "v$Version"
$tagRef = git ls-remote origin "refs/tags/$tag" "refs/tags/$tag^{}"
if ($tagRef) {
    $resolved = (($tagRef | Select-Object -Last 1) -split '\s+')[0]
    if ($resolved -ne $commit) { throw 'Existing release tag points to another commit.' }
}
$releases = Invoke-RestMethod "$api/releases?per_page=100" -Headers $headers
$release = $releases | Where-Object tag_name -EQ $tag | Select-Object -First 1
if ($release -and !$release.draft) { throw 'A published release already exists. It will not be modified.' }
if (!$release) {
    $body = @{ tag_name=$tag; target_commitish=$commit; name="TrafficMonitor $tag (Codex)"; body=[IO.File]::ReadAllText((Join-Path $root $NotesFile)); draft=$true; prerelease=$false } | ConvertTo-Json
    $release = Invoke-RestMethod "$api/releases" -Method Post -Headers $headers -ContentType 'application/json' -Body ([Text.Encoding]::UTF8.GetBytes($body))
}
Write-Output "Draft release: https://github.com/Francis0927/TrafficMonitor/releases/tag/$tag"
foreach ($name in $required) {
    $file = Join-Path $ArtifactDirectory $name
    $hash = (Get-FileHash $file -Algorithm SHA256).Hash.ToLowerInvariant()
    $assets = Invoke-RestMethod "$api/releases/$($release.id)/assets" -Headers $headers
    $asset = $assets | Where-Object name -EQ $name | Select-Object -First 1
    if (!$asset) {
        $upload = ($release.upload_url -replace '\{.*$', '') + '?name=' + [Uri]::EscapeDataString($name)
        $asset = Invoke-RestMethod $upload -Method Post -Headers $headers -ContentType 'application/octet-stream' -InFile $file
    }
    if ($asset.size -ne (Get-Item $file).Length) { throw "Remote size mismatch; draft preserved: $name" }
    # Download the asset bytes and verify SHA-256 even when GitHub supplies a digest.
    $download = Join-Path $ArtifactDirectory "verify-upload-$name"
    $downloadHeaders = $headers.Clone(); $downloadHeaders.Accept = 'application/octet-stream'
    Invoke-WebRequest "$api/releases/assets/$($asset.id)" -Headers $downloadHeaders -OutFile $download
    if ((Get-FileHash $download -Algorithm SHA256).Hash.ToLowerInvariant() -ne $hash) { throw "Remote SHA-256 mismatch; draft preserved: $name" }
    Write-Output "SHA-256 verified: $name"
}
$remoteAssets = Invoke-RestMethod "$api/releases/$($release.id)/assets" -Headers $headers
if (@(Compare-Object $required @($remoteAssets.name)).Count) { throw 'Draft has unexpected or missing attachments; it was preserved.' }
$published = Invoke-RestMethod "$api/releases/$($release.id)" -Method Patch -Headers $headers -ContentType 'application/json' -Body '{"draft":false,"prerelease":false,"make_latest":"true"}'
if ($published.draft -or $published.prerelease) { throw 'Release publication was not confirmed.' }
Write-Output "Published: $($published.html_url)"
