# Onboarding（新人上手指南）

## 目标

让你在第一天完成以下三件事：

1. 了解仓库结构与关键文档。
2. 完成一次标准 Git 协作流程。
3. 输出一个可被 review 的最小 PR。

## 第一步：理解仓库

优先阅读：

- `README.md`：项目全局说明。
- `docs/onboarding.md`：上手步骤。
- `docs/architecture.md`：系统结构（当前为占位，后续补充）。
- `docs/development.md`：开发规范（当前为占位，后续补充）。

## 第二步：完成一次 Git 流程

```bash
# 1) 新建功能分支
git checkout -b feature/your-topic

# 2) 提交改动
git add .
git commit -m "docs: add onboarding notes"

# 3) 推送并创建 PR
git push -u origin feature/your-topic
```

## 第三步：提交最小 PR（建议内容）

可选一个简单任务：

- 修正 README 的错别字；
- 补充一个 FAQ；
- 为 docs 增加一段 troubleshooting；
- 添加一个最小测试样例（后续有代码时）。

## 常见建议

- 小步提交：每次只解决一个小问题。
- 优先可读性：变量、函数、注释命名清晰。
- 先写验证步骤：在 PR 中明确“如何复现/如何验证”。
- 卡住及时同步：不要在一个问题上沉没太久。
