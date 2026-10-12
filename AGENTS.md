# TrafficMonitor fork 维护约定

- 本仓库为 `Francis0927/TrafficMonitor`，参考项目 CodexSpeedVisualizer 只读使用。
- Codex 功能为内置 C++ 功能；不引入必须同时运行的应用或 Node.js 服务。
- 任务栏中的 Codex 项目只显示输出速度，额度和本地重置时间放在鼠标提示中。
- 不读取账户凭据，不修改 Codex 日志或本机遗留配置；缺失额度保持未知。
- 完整发布规则见 `docs/RELEASE_RULES.md`。唯一发布渠道为本仓库正式 GitHub Releases，标签 `v版本`；干净且已推送的提交才可发布。
- 在隔离源码中构建并验证真实版本、ZIP 载荷和 SHA-256；通过草稿上传，下载核验全部附件后正式发布。
- 同版本附件不覆盖，历史 Release 不改写；输入改变必须递增版本。失败保留原产物和草稿。
- 修改后运行 `scripts/test-codex.ps1`；发布前还需通过完整 MFC 构建。维护 README、CHANGELOG、修改说明和版本发布说明。
