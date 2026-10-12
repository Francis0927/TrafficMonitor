# TrafficMonitor fork 发布规则

唯一发布和更新渠道为 [Francis0927/TrafficMonitor 的 GitHub Releases](https://github.com/Francis0927/TrafficMonitor/releases)。只发布正式版本，标签为 `v主版本.次版本.修订版本`；不发布到共享目录或 Gitee，不改写上游或历史 Release。

## 发布前

1. 完成功能和文档维护，运行 `./scripts/test-codex.ps1`。
2. 新构建输入必须递增版本。同步 `stdafx.h` 的 `VERSION`、`TrafficMonitor.rc` 的四段资源版本（例如 `1.87.0.0`）、脚本/工作流默认版本、README、CHANGELOG 和 `docs/releases/v版本.md`。
3. 提交到本仓库并推送；工作区必须干净，GitHub 对应分支必须指向当前提交。标签必须指向该提交，现有同名标签不能移动。
4. 凭据仅从 `GH_TOKEN`、`GITHUB_TOKEN` 或 Git 凭据管理器读取，不写仓库、发布包或日志。GitHub Actions 使用自动提供的仓库令牌。

## 构建

在 Visual Studio 2022 安装 C++、MFC、Windows SDK 和 v143 工具集后运行：

```powershell
./scripts/build-release.ps1 -Version 1.87.0
```

脚本从当前已提交的 Git 树归档到 `artifacts/release-v版本-提交/source`，在隔离目录运行测试并构建 Lite 配置的 x64、x86 原生便携版本。该版本与上游 Lite 一致，不包含温度监控；Codex 功能在两种架构中均可用。不生成安装包、ARM64EC 包或 Windows XP 包。

只允许以下正式附件：

- `TrafficMonitor-v版本-x64.zip`
- `TrafficMonitor-v版本-x86.zip`
- `SHA256SUMS.txt`
- `README.txt`

ZIP 根目录直接包含 EXE、Visual C++ CRT/MFC 可再发行 DLL、语言、皮肤、许可证和说明。不得打包调试文件、开发源码、用户日志、账户凭据、本地配置或废弃更新地址。

核验真实 EXE 文件版本、GitHub 更新默认行为、ZIP 全部载荷的数量和逐文件 SHA-256，生成校验清单。`commit.txt`、构建日志和隔离源码只用于构建追溯，不上传到 Release。打包失败保留产物和日志，不清理旧版本。

## 正式发布

```powershell
./scripts/publish-github.ps1 -Version 1.87.0 -NotesFile docs/releases/v1.87.0.md
```

脚本校验干净工作区、GitHub 提交、产物提交、版本标签及本地校验清单，先创建草稿，再上传全部四个附件。每个远端附件必须大小一致且下载核验 SHA-256 一致，随后才正式发布并设为最新版本。

同版本附件不覆盖；重试只能复用字节一致的附件。发布失败保留草稿和已验证的产物，报告失败步骤。重试使用原目录的包；不要重新构建同版本。已正式发布的 Release 不修改，修正必须生成下一版本。

也可以在 GitHub Actions 运行 **Codex release**：默认只构建；勾选 `publish` 后，自动执行上述草稿上传和校验流程。失败时保留 Actions 产物和构建日志。重试上传应下载原工作流产物后使用 `-ArtifactDirectory` 参数；相同提交重新构建可能因编译时间而改变哈希。

## 安装和客户端更新

用户下载对应架构 ZIP，解压运行。默认启用检查更新，读取本 fork 的最新正式 Release，确认后打开对应下载链接，由用户退出程序并替换文件；这不是自动覆盖安装流程。保留用户原配置。上游版本用户手动迁移一次，之后检查更新只使用本仓库。

## 本地清理

当前版本保留可追溯源码、构建日志和原始包，不自动删除历史目录。人工清理只处理已明确识别且不在运行的 `artifacts` 临时构建目录，不跟随符号链接或目录联接，不删除用户便携运行目录、未知文件或更高版本。CodexSpeedVisualizer 的清理脚本只适用于其自身仓库，不对 TrafficMonitor 执行。
