# 双人协作约定

成员：[@Semt0](https://github.com/Semt0)、[@VVKAHPM](https://github.com/VVKAHPM)。
具体分工是建议，双方可交换；完整任务见 [实现计划](docs/implementation-plan.md)。

## 分支与合并

1. 从最新 `main` 建短期功能分支，例如 `feat/transport`、`feat/channel`、`feat/sftp`。
2. 每次 PR 尽量只完成一个阶段或一类修复；说明行为、测试命令、结果及未完成项。
3. 对方 review 后合并；共享头文件、返回值、内存归属变化要在 PR 中明确写出。
4. 每次合并检查编译，能连接服务器后检查此前已经通过的 CP。
5. `origin` 为本协作仓库；`upstream` 为课程仓库。不要将作业实现推到课程仓库。

```sh
git clone https://github.com/Semt0/lab-sftp.git
cd lab-sftp
git remote add upstream https://github.com/SOAR-PKU/lab-sftp.git
git switch -c feat/your-task
```

## 建议文件归属

- Semt0：`session.c`、`socket.c`、`packet.c`、`kex.c`、`dh.c`、`auth.c`，PT1–PT4。
- VVKAHPM：`channel.c`、`sftp.c`、`client.c`，PT5–PT6。
- 共同确认：`include/libsftp/`、`buffer.c`、`string.c`、加密公共工具、构建配置和最终提交材料。

底层尚未打通时，上层可以通过模拟 packet/channel 输入输出测试；模拟通过不算真实服务器 CP 验收。
接口以计划中的契约为目标，涉及当前行为变更时先协调，再一起更新调用方。

## 测试与材料

- 当前尚未启用 GitHub Actions：登录凭据缺少 workflow 权限。Linux 编译配置保存在 `docs/build-workflow.yml`；有相应权限后将其移动至 `.github/workflows/build.yml` 并推送。即使编译通过，也不证明协议实现正确。
- CP 证据放在 `checkpoint/cp1/` 至 `checkpoint/cp6/`，每份注明日期、提交号、命令、服务器版本及实际结果。
- 公开仓库不提交密码、私钥、会话密钥或包含它们的调试日志；使用测试账号和测试文件。
- 保留上游许可证和作者信息。课程要求自行理解并实现；AI 可辅助解释、审查和定位问题。
