# CP3 — NEWKEYS 与后续加密流量

**验收结果：通过。** 2026-11-18，在 macOS 的 `lo0` 上捕获了完整的版本交换、密钥交换和第一对加密报文。[原始抓包](loopback.pcap) 的 TShark 解析与 [终端录制](typescript) 中的解析结果一致。整理时仓库 HEAD 为 `b59138e`；录制本身未打印构建版本或二进制标识。

本次使用 [CP2 的实验 sshd](../cp2/README.md)，服务端为 `127.0.0.1:22`（OpenSSH 10.5）。录制显示协商了 `diffie-hellman-group14-sha256`、`ssh-rsa`、双向 `aes256-ctr` 和 `hmac-sha1`，无压缩。

## 关键帧

四帧均属于 `tcp.stream == 0`。两端 IP 都是 `127.0.0.1`，用 TCP 端口判断方向。

| 帧号 | 方向（源端口 → 目的端口） | Wireshark / TShark 观察 |
| --- | --- | --- |
| 15 | 服务器 → 客户端（`22 → 56954`） | 同一帧包含 DH Reply（类型 `31`）和服务器的 `NEWKEYS`（类型 `21`） |
| 17 | 客户端 → 服务器（`56954 → 22`） | 客户端的 `NEWKEYS`（类型 `21`） |
| 19 | 客户端 → 服务器（`56954 → 22`） | 客户端在帧 17 之后的加密报文，TCP 载荷 52 字节，包长字段也已加密。见 [客户端截图](wireshark-client-encrypted-frame-19.png)。 |
| 21 | 服务器 → 客户端（`22 → 56954`） | 服务器在帧 15 之后的加密报文，TCP 载荷 52 字节，包长字段也已加密。见 [服务器截图](wireshark-server-encrypted-frame-21.png)。 |

终端录制中的客户端调试日志显示双方切换密钥后收发了 `SERVICE_REQUEST`（类型 `5`）和 `SERVICE_ACCEPT`（类型 `6`）。按方向和顺序，这两条消息对应帧 19 和 21；类型来自客户端日志，抓包在网络上只显示加密报文。录制停在密码提示，没有进行密码认证。

## 复现

从项目根目录在抓包终端运行：

```sh
sudo /usr/sbin/tcpdump -i lo0 -nn -s 0 -U \
  -w checkpoint/cp3/loopback.pcap 'host 127.0.0.1 and tcp port 22'
```

看到开始监听后，另开终端，在项目根目录运行 `/usr/bin/script checkpoint/cp3/typescript`。在录制会话中执行 `./build/client username@127.0.0.1`，将 `username` 换成实验账号。出现 `password:` 后按 `Ctrl-C`，无需输入密码。回到抓包终端按 `Ctrl-C` 停止抓包，录制会话先保持打开。复跑请使用新的抓包和录制文件名，避免覆盖已有证据。

用 Wireshark 打开 `loopback.pcap`，先以 `ssh.message_code == 21` 查找双方的 `NEWKEYS`。点开 TCP 详情确认它们属于同一条流，再用 `tcp.stream == N && (ssh.message_code == 21 || ssh.encrypted_packet)` 展示切换前后的包，将 `N` 换成实际流编号。回环连接的两端 IP 相同，用端口判断方向：源端口为 `22` 是服务器发出，目的端口为 `22` 是客户端发出。

在仍打开的录制会话中执行以下命令（本次流编号为 `0`，重跑时以实际编号为准），再输入 `exit` 结束录制：

```sh
tshark -r checkpoint/cp3/loopback.pcap \
  -Y 'tcp.stream == 0 && (ssh.message_code == 21 || ssh.encrypted_packet)' \
  -T fields -E header=y -e frame.number -e tcp.stream \
  -e tcp.srcport -e tcp.dstport -e ssh.message_code -e _ws.col.Info
```

`NEWKEYS` 本身仍用旧状态发送；**同一个发送方向**在它之后才用新密钥。核对每个方向在自己的 `NEWKEYS` 之后的加密数据。客户端日志中的消息类型是程序加解密时看到的内容，抓包中的 `Encrypted Packet` 才是线上观察。若同一帧同时包含 `NEWKEYS` 和紧接的密文，展开 SSH 详情核对顺序，不要求它们一定占不同帧。
