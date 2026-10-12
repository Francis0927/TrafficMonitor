# Fork 修改说明

基于 [zhongyang219/TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor)，保留原项目许可证、作者信息和历史更新日志。

## 1.87.0 · 2026-10-12

| 修改 | 位置 | 行为 |
| --- | --- | --- |
| 原生 Codex 日志采集 | `TrafficMonitor/CodexTelemetry.*` | 后台增量读取本地会话，平滑输出速度、额度快照、重复事件过滤 |
| 任务栏内置项目 | `DisplayItem.*` / `CommonData.h` | 新增 Codex 输出速度，新配置默认仅此项 |
| 悬停详情 | `CodexTelemetryView.*` / 两个窗口 | 五小时和一周额度、本地重置/采集时间、未知或过期状态 |
| 本仓库更新 | `UpdateHelper.cpp` / `ReleaseVersion.h` | GitHub 正式 Releases API、架构匹配、数字版本比较，不读取上游或 Gitee 更新清单 |
| 网络响应读取 | `Common.cpp` | 直接读取原始响应字节，避免 JSON 被文本转换截断 |
| 本地化 | `language.h` / `language/*.ini` | 简体、繁体中文；其他语言的新字段暂用英文 |
| 构建与发布 | `scripts` / `.github/workflows/codex-release.yml` | 隔离构建、包版本/载荷校验、草稿上传、下载核对 SHA-256、正式发布 |
| 文档 | README / CODEX / RELEASE_RULES / CHANGELOG | fork 来源、使用步骤、维护说明、发布渠道和验证流程 |

JSON 解析使用固定版本的 [nlohmann/json 3.12.0](https://github.com/nlohmann/json/releases/tag/v3.12.0)，源文件校验值和 MIT 许可保存在 `third_party/nlohmann`。保留原项目的硬件监控插件、皮肤和插件下载参考链接。

## 维护

- 上游变化通过独立提交合入，Codex 功能保持内置且不依赖另一个应用进程。
- 日志格式发生变化时先增加真实格式的匿名测试样例，再改解析；缺失数据必须保持未知状态。
- 修改显示项目时只在枚举末尾追加，不能改变原枚举值；保存的排序索引须迁移。
- 新版本同时维护程序 `VERSION`、Windows 资源版本、发布说明、发布脚本/工作流的默认版本、双语 README 和 CHANGELOG。
- 上游历史日志不改写；本 fork 的变更记录在 `CHANGELOG.md`。
