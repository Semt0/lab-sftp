# Checkpoint 证据清单

当前尚未完成任何运行验收。下表为待收集材料，不是结果。

| 目录 | 验收要求 | 建议记录 |
| --- | --- | --- |
| `cp1/` | 展示服务器 identification string | 客户端日志和服务器版本 |
| `cp2/` | 展示协商后的算法套件 | KEX、host key、两个方向的 cipher/MAC；写明算法要求差异如何解决 |
| `cp3/` | 展示密钥交换后消息加密 | NEWKEYS 前后抓包截图及说明 |
| `cp4/` | 服务器允许客户端登录 | 认证成功记录；另外测试错误密码后重试 |
| `cp5/` | 成功打开 channel | 双方 channel ID、window、maxpacket；SFTP subsystem 成功回复 |
| `cp6/` | 上传及下载 `client.c` | 两端记录、下载到独立目录、原文件/远端/下载副本的哈希对比 |

每个目录增加简短说明：测试日期、Git commit、操作系统、编译器/OpenSSL、服务器版本、算法配置、命令、预期与实际结果、附件路径。课程允许文字、图片、视频或终端录制，均需简短解释。

本目录 README 不代替真实证据。最终归档需包含整个工程、`checkpoint/` 和 `not-implemented.pdf`；后者必须据实际结果填写。全完成时，课程 README 要求提交空文件。
