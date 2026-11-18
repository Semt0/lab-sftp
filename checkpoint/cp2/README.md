# CP2 — SSH 算法协商

2026-11-18，macOS 本机回环测试，服务端标识为 `SSH-2.0-OpenSSH_10.5`。整理时源码 HEAD 为 `b59138e`；录制未输出构建版本或二进制标识。

[typescript](typescript) 与 CP1 来自同一次运行，已去除个人信息。客户端输出以下协商结果，随后打印 `kex negotiation succeed`：

| 项目 | 客户端日志中的结果 |
| --- | --- |
| 密钥交换 | `diffie-hellman-group14-sha256` |
| 主机密钥 | `ssh-rsa` |
| 双向加密 | `aes256-ctr` |
| 双向 MAC | `hmac-sha1` |
| 双向压缩 | `none` |

录制还显示双方交换了 `NEWKEYS` 并进入密码提示；本次没有提交密码。CP2 以协商结果和成功日志为验收证据。

## 复现

端口 `22` 空闲后，在独立终端运行以下命令并保持该终端打开。把 `username` 换成实验账号。该实验实例不读系统 `sshd` 配置，只监听 `127.0.0.1:22`，并指定与客户端一致的套件。

```sh
sudo /usr/sbin/sshd -D -e -f /dev/null \
  -h /etc/ssh/ssh_host_rsa_key -p 22 \
  -o ListenAddress=127.0.0.1 -o AllowUsers=username \
  -o KexAlgorithms=diffie-hellman-group14-sha256 \
  -o HostKeyAlgorithms=ssh-rsa \
  -o Ciphers=aes256-ctr -o MACs=hmac-sha1 \
  -o PasswordAuthentication=yes -o AuthenticationMethods=password \
  -o UsePAM=yes -o DisableForwarding=yes \
  -o 'Subsystem=sftp internal-sftp'
```

另开终端，在项目根目录运行 `/usr/bin/script checkpoint/cp2/typescript-rerun`，再运行 `./build/client username@127.0.0.1`。看到协商结果和成功日志后，可在密码提示处按 `Ctrl-C`，然后输入 `exit` 结束录制。
