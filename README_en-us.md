**[简体中文](./README.md) | English**

# TrafficMonitor · Codex fork

[![Windows CI](https://github.com/Francis0927/TrafficMonitor/actions/workflows/main.yml/badge.svg)](https://github.com/Francis0927/TrafficMonitor/actions/workflows/main.yml)

A fork of [zhongyang219/TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor), retaining Windows network, CPU, memory, taskbar, skin and plugin monitoring, with native **Codex output speed and allowance**. This fork is independently maintained; the upstream author is not responsible for its changes.

Current source version: **1.87.0**. Availability is determined by [this repository's Releases](https://github.com/Francis0927/TrafficMonitor/releases), not by the source version alone.

## Use

1. Download the x64 or x86 ZIP from [Releases](https://github.com/Francis0927/TrafficMonitor/releases/latest) and verify `SHA256SUMS.txt`.
2. Extract and run `TrafficMonitor.exe`, retaining the bundled CRT/MFC DLLs.
3. Right-click > Show taskbar window > Display settings. Select only **Codex output speed**.
4. Hover to view 5-hour and weekly remaining allowance, local reset times and the quota sample time. Keep tooltips enabled.

New configurations select only Codex speed for the taskbar; existing display selections are retained. The main floating window retains its skin and also exposes Codex details in its tooltip.

The collector reads `sessions` and `archived_sessions` under `CODEX_HOME`, falling back to `%USERPROFILE%\.codex`. It runs in a separate background thread and uses a 15-second EMA of output tokens. No plugin, Node.js or second application is needed. Allowance comes from the latest local log sample, not a live account request. Missing values stay unknown; passed reset times are marked as awaiting a new sample. It does not read authentication files or upload conversations.

Portable releases target Windows 10/11 and use the upstream Lite configuration without built-in temperature monitoring. Original skins and [plugins](https://github.com/zhongyang219/TrafficMonitorPlugins) remain available.

## Updates and documentation

Update checks use only [Francis0927/TrafficMonitor stable GitHub Releases](https://github.com/Francis0927/TrafficMonitor/releases/latest), enabled by default. Confirming an update opens the matching download; exit before replacing the program and keep your configuration. Upstream users must migrate manually once.

- [Codex usage, data semantics and troubleshooting](./docs/CODEX.md) (Chinese)
- [Fork changes and maintenance](./docs/CHANGES.md) (Chinese)
- [Release rules](./docs/RELEASE_RULES.md) (Chinese)
- [Fork changelog](./CHANGELOG.md) / [upstream history](./UpdateLog/update_log_en-us.md)
- [Original feature FAQ](./Help_en-us.md) / [upstream Wiki](https://github.com/zhongyang219/TrafficMonitor/wiki/Home_en)

## Build

Full builds require Visual Studio 2022 with C++, MFC, Windows SDK and v143. Collector tests require only C++ tools.

```powershell
./scripts/test-codex.ps1
# Commit and push before building release packages
./scripts/build-release.ps1 -Version 1.87.0
./scripts/publish-github.ps1 -Version 1.87.0 -NotesFile docs/releases/v1.87.0.md
```

The **Codex release** Actions workflow builds isolated x64/x86 ZIPs. Publishing is optional and uploads a draft first, then downloads and checks every attachment's SHA-256 before publication. Existing release assets are never overwritten.

## Credits

Original author: zhongyang219. The original [Anti-996 license](./LICENSE) is retained. The Codex algorithm refers to [CodexSpeedVisualizer](https://github.com/Francis0927/CodexSpeedVisualizer). [nlohmann/json 3.12.0](https://github.com/nlohmann/json/releases/tag/v3.12.0) is vendored with its MIT license. Historical screenshots remain in `Screenshots`.