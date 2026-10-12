**简体中文 | [English](./README_en-us.md)**

# TrafficMonitor · Codex fork

[![Windows CI](https://github.com/Francis0927/TrafficMonitor/actions/workflows/main.yml/badge.svg)](https://github.com/Francis0927/TrafficMonitor/actions/workflows/main.yml)
[![Release](https://img.shields.io/github/v/release/Francis0927/TrafficMonitor)](https://github.com/Francis0927/TrafficMonitor/releases/latest)

这是 [zhongyang219/TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) 的 fork，保留 Windows 网速、CPU、内存、任务栏窗口、皮肤和插件功能，新增原生 **Codex 输出速度与额度显示**。本 fork 独立发布，原作者不负责本 fork 的修改。

当前开发版本：**1.87.0**。正式下载以 [本仓库 GitHub Releases](https://github.com/Francis0927/TrafficMonitor/releases) 为准；源码版本不代表已发布。

## Codex 功能

- 任务栏显示 `Codex: 速度 token/s`，默认只选择速度项目。
- 悬停显示五小时、一周剩余额度、各自重置时间及额度采集时间。
- 只读本地 Codex 日志，支持 `CODEX_HOME` 和 `%USERPROFILE%\.codex`。
- 后台增量读取，采用 15 秒指数平滑；无需插件、Node.js 或同时运行 CodexSpeedVisualizer。
- 缺失额度显示“暂无数据”，重置时间过去后提示等待新数据，不猜测剩余值。

额度来自本机日志的最后快照，其他设备的用量需要等待新的本地日志反映。[数据口径和常见问题](./docs/CODEX.md)。

## 下载与使用

1. 从 [Releases](https://github.com/Francis0927/TrafficMonitor/releases/latest) 下载 x64 或 x86 ZIP，并核对 `SHA256SUMS.txt`。
2. 解压运行 `TrafficMonitor.exe`，保留 ZIP 内附带的运行库 DLL。
3. 右键选择“显示任务栏窗口”，打开任务栏“显示设置”，仅勾选“Codex 输出速度”。
4. 保持鼠标提示开启；悬停查看额度和刷新时间。

新配置的任务栏默认只选择 Codex 速度。旧配置保留原项目选择，需要按步骤 3 切换。原有悬浮窗和皮肤继续可用，悬浮窗鼠标提示同样包含 Codex 详情。

发布包面向 Windows 10/11，使用上游 Lite 构建，不含内置温度监控。若需要温度监控，可参考原作者的 [硬件监控插件](https://github.com/zhongyang219/TrafficMonitorPlugins)。

## 更新

本 fork 只检查 [Francis0927/TrafficMonitor 正式 Releases](https://github.com/Francis0927/TrafficMonitor/releases/latest)。启动检查默认开启；发现新版本后打开对应架构下载链接，由用户退出程序后替换文件，保留配置。上游用户需手动迁移一次。Gitee、网盘和内网共享不是本 fork 的发布渠道。

## 文档

- [Codex 使用与数据说明](./docs/CODEX.md)
- [Fork 修改说明与维护约定](./docs/CHANGES.md)
- [更新日志](./CHANGELOG.md) / [上游历史日志](./UpdateLog/update_log.md)
- [发布版本规则](./docs/RELEASE_RULES.md)
- [原有功能常见问题](./Help.md)
- [上游功能 Wiki](https://github.com/zhongyang219/TrafficMonitor/wiki)
- [皮肤制作教程](./皮肤制作教程.md)

## 构建与发布

完整构建需要 Visual Studio 2022、C++、MFC、Windows SDK 和 v143 工具集。独立采集测试只需要 C++ 工具。

```powershell
./scripts/test-codex.ps1
# 提交并推送后构建隔离包
./scripts/build-release.ps1 -Version 1.87.0
./scripts/publish-github.ps1 -Version 1.87.0 -NotesFile docs/releases/v1.87.0.md
```

也可使用 GitHub Actions 的 **Codex release** 工作流，默认仅构建，勾选 `publish` 后按规则发布。全部附件通过远端 SHA-256 校验后才将草稿转为正式版。

## 来源与许可

原项目作者：zhongyang219；原项目 [Anti-996 许可](./LICENSE) 与 [中文许可](./LICENSE_CN) 保留。Codex 日志算法参考 [Francis0927/CodexSpeedVisualizer](https://github.com/Francis0927/CodexSpeedVisualizer)。JSON 解析器 [nlohmann/json 3.12.0](https://github.com/nlohmann/json/releases/tag/v3.12.0) 使用 MIT 许可，附带原始许可文件。原项目截图位于 `Screenshots`，新增功能以当前说明为准。