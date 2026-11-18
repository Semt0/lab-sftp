# Checkpoint 证据清单

每个 CP 提交一份用 `script` 生成的 `typescript`，并附一份简短的 `README.md`，指出哪几行是通过该 CP 的证据。下表列的是交付要求。

附件来自实际验收录制。提交版将实验账号、机器名和个人路径替换为通用占位符，清理终端提示符和启动杂项，并省略可推算密码长度的调试字段；保留原录制行号、验收结果、哈希和抓包帧号。复现时将 `username` 换成实验服务允许登录的账号。

| 目录 | 验收要求 | 建议记录 |
| --- | --- | --- |
| `cp1/` | 展示服务器 identification string | 客户端输出的实际服务器标识 |
| `cp2/` | 展示协商后的算法套件 | KEX、host key、双向 cipher/MAC 和协商成功日志 |
| `cp3/` | 展示密钥交换后消息加密 | 用终端录制 `tshark` 对同一 TCP 流的解析，标出双方的 NEWKEYS 与随后的 Encrypted Packet；可附原始抓包 |
| `cp4/` | 服务器允许客户端登录 | 客户端的密码认证成功日志 |
| `cp5/` | 成功打开 channel | 双方 channel ID、window、maxpacket；子系统请求成功可用后续 SFTP 版本交换补证 |
| `cp6/` | 上传及下载 `client.c` | 录下上传、下载和哈希校验；在独立目录下载，避免覆盖原文件 |

在项目根目录运行 `script checkpoint/cp1/typescript` 开始录制，执行该项验收命令后输入 `exit` 结束。其他 CP 替换目录名。记录测试日期、源码版本、客户端和服务端环境、命令及观察到的结果。只录终端输出；macOS 不用 `-k` 或 `-r`，Linux 不用 `--log-in` 或 `--log-io`。

本目录 README 不代替真实证据。最终归档需包含整个工程、`checkpoint/` 和 `not-implemented.pdf`；后者必须据实际结果填写。全完成时，课程 README 要求提交空文件。
