# MY-FIRST-PROJECT

这是一个用于演示和学习的项目仓库。当前阶段重点是建立**新人友好**的工程基础：明确目录结构、统一协作规范、提供可执行的学习路径。

## 1. 项目目标

- 为团队提供一个从 0 到 1 的项目模板。
- 让新人在最短时间内完成：克隆仓库 -> 了解结构 -> 本地开发 -> 提交 PR。
- 为后续功能开发预留标准化目录和文档位置。

## 2. 推荐目录结构

```text
.
├── README.md                 # 项目总览（你正在阅读）
├── docs/
│   ├── onboarding.md         # 新人上手指南
│   ├── architecture.md       # 架构说明（后续补充）
│   └── development.md        # 开发流程与规范（后续补充）
├── src/                      # 业务源码（按模块继续细分）
├── tests/                    # 自动化测试
└── .gitignore                # Git 忽略规则
```

> 说明：`src/` 与 `tests/` 已预留，便于后续直接落地代码与测试。

## 3. 新人必读内容

建议按顺序阅读：

1. `README.md`：了解项目目标与整体结构。
2. `docs/onboarding.md`：按步骤完成本地环境准备与协作流程。
3. `docs/architecture.md`：理解系统模块边界与设计原则（后续补全）。
4. `docs/development.md`：遵循开发规范、提交规范和评审要求（后续补全）。

## 4. 协作约定（初版）

- 分支命名：`feature/<topic>`、`fix/<topic>`。
- 提交信息建议使用 Conventional Commits，例如：
  - `feat: add user profile module`
  - `fix: correct login validation`
  - `docs: improve onboarding guide`
- PR 要求：
  - 描述背景与改动点。
  - 附带测试/验证结果。
  - 如涉及界面改动，附截图。

## 5. 后续学习建议

- 先跑通流程：会拉代码、会提分支、会提 PR。
- 再做最小任务：选择一个小需求完整走完开发闭环。
- 每次改动都补文档：让“个人经验”变成“团队资产”。
- 每周复盘：记录本周问题、解决方式与待改进点。

## 6. 下一步建议（Maintainer 待办）

- [ ] 在 `src/` 中初始化第一版业务模块。
- [ ] 在 `tests/` 中添加基础测试样例。
- [ ] 在 `docs/architecture.md` 补充架构图与模块职责。
- [ ] 在 `docs/development.md` 补充本地运行、测试与发布流程。
