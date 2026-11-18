# CP5 — SSH 通道打开与 SFTP 子系统

**验收结果：通过。** 2026-11-18，客户端连接 macOS 本机的实验 OpenSSH 10.5 服务（`127.0.0.1:22`），录制记录的源码 HEAD 为 `b59138e`。[typescript](typescript) 与 [CP4 录制](../cp4/typescript) 字节一致；同一次运行先通过密码认证，再打开通道和请求 SFTP 子系统。

| 录制行号 | 实际观察 |
| --- | --- |
| 36、38 | 客户端发送 `SSH_MSG_CHANNEL_OPEN`（90），收到 `SSH_MSG_CHANNEL_OPEN_CONFIRMATION`（91）。 |
| 39 | 客户端通道 ID 为 `1`，初始接收窗口为 `64000` 字节；服务器通道 ID 为 `0`，初始接收窗口为 `0` 字节。两端公布的最大通道包长度均为 `32768` 字节。 |
| 40–43 | 客户端发送子系统请求 `SSH_MSG_CHANNEL_REQUEST`（98）。等待期间收到 `SSH_MSG_CHANNEL_WINDOW_ADJUST`（93），服务器接收窗口增至 `2097152` 字节。随后收到 `SSH_MSG_CHANNEL_SUCCESS`（99），确认子系统请求成功。 |
| 44–47 | 双向收发 `SSH_MSG_CHANNEL_DATA`（94），随后出现 `sftp>`。结合客户端只有在 `sftp_init()` 成功之后才进入交互循环，这表明 SFTP v3 初始化通过。 |

第 40 行的 DEBUG 日志只显示消息类型；请求的是 `subsystem=sftp` 由客户端的 `ssh_channel_request_sftp()` 调用确定。第 43 行证实服务端接受该请求，不能仅靠通道打开日志推断子系统已就绪。本次在提示符处输入 `bye` 退出，没有上传或下载文件。

## 复现

沿用 [CP2 的实验 sshd](../cp2/README.md)。按 [CP4 的录制步骤](../cp4/README.md) 运行自写客户端，将录制文件路径改为 `checkpoint/cp5/typescript-rerun`。输入实验账号密码，等到出现 `sftp>`，输入 `bye` 并回车。核对 91、99 和 `sftp>` 三项证据。
