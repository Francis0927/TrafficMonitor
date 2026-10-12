TrafficMonitor 1.87.0 - Codex fork
Repository and updates: https://github.com/Francis0927/TrafficMonitor
Downloads: https://github.com/Francis0927/TrafficMonitor/releases/latest

1. Choose x64 for 64-bit Windows, x86 for 32-bit Windows; extract the ZIP.
2. Run TrafficMonitor.exe. CRT/MFC runtime DLLs are included.
3. Right-click > Show taskbar window > Display settings. Select only Codex output speed.
4. Hover over the taskbar to see 5-hour/weekly remaining allowance and local reset times.
5. Quotas are the last values recorded by local Codex, not a live account query.
6. CODEX_HOME overrides %USERPROFILE%\.codex. No Node.js or second application is needed.
7. Updates check this fork's stable GitHub Releases. Exit before replacing program files.
8. Existing configuration is retained. Missing quota/log data is shown as unknown.

Supported target: Windows 10/11, Lite build (no built-in temperature monitoring).
Verify ZIP and README SHA-256 against SHA256SUMS.txt before use.
User guide: https://github.com/Francis0927/TrafficMonitor/blob/master/docs/CODEX.md
Changes: https://github.com/Francis0927/TrafficMonitor/blob/master/CHANGELOG.md

Original project: https://github.com/zhongyang219/TrafficMonitor
Original Anti-996 license retained: LICENSE and LICENSE_CN.
JSON parser: nlohmann/json 3.12.0, MIT; see nlohmann-json-LICENSE.MIT.
Microsoft Visual C++ runtime DLLs are redistributed from Visual Studio's Redist directory.
