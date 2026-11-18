# CP1 — SSH 版本交换

2026-11-18，macOS 本机回环测试。客户端连接 `127.0.0.1:22`，服务端报告 `SSH-2.0-OpenSSH_10.5`。整理记录时仓库源码的 HEAD 为 `b59138e`；录制未输出构建版本或二进制标识。

```sh
./build/client username@127.0.0.1
```

正式附件 [typescript](typescript) 由 `script` 录制。日志显示本端已发送标识，并从服务端读到 `server identification: SSH-2.0-OpenSSH_10.5`，满足 CP1。此次服务端配置见 [CP2 复现说明](../cp2/README.md)；同一份录制也用于 CP2。

[初次终端输出](terminal.txt) 是配置兼容算法之前的手动摘录，仅保留作排查参考。
