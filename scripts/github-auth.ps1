function Get-GitHubHeaders {
    $token = $env:GH_TOKEN
    if (!$token) { $token = $env:GITHUB_TOKEN }
    if (!$token) {
        $oldPrompt = $env:GIT_TERMINAL_PROMPT; $oldInteractive = $env:GCM_INTERACTIVE
        try {
            $env:GIT_TERMINAL_PROMPT = '0'; $env:GCM_INTERACTIVE = 'never'
            $credential = "protocol=https`nhost=github.com`n`n" | git credential fill 2>$null
            $password = $credential | Where-Object { $_.StartsWith('password=') } | Select-Object -First 1
            if ($password) { $token = $password.Substring(9) }
        } finally { $env:GIT_TERMINAL_PROMPT = $oldPrompt; $env:GCM_INTERACTIVE = $oldInteractive }
    }
    if (!$token) { throw 'No GitHub credential in GH_TOKEN, GITHUB_TOKEN, or Git credential manager.' }
    return @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github+json'; 'X-GitHub-Api-Version' = '2022-11-28'; 'User-Agent' = 'TrafficMonitor-release' }
}
