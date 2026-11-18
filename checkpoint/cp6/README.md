# CP6 — 上传并下载 client.c

**验收结果：通过。** 2026-11-18，在 macOS 上使用自写客户端的 `NOTICE` 版连接实验 OpenSSH 10.5（`127.0.0.1:22`）。同一会话上传和下载了 `client.c`。核对时的源码 HEAD 为 `b59138e`；录制未打印提交号。

| [终端录制](typescript)行号 | 证据 |
| --- | --- |
| 5 | 传输前的仓库原文件 SHA-256。 |
| 17 | 密码提示后直接出现认证成功日志。未发现密码回显，录制也没有认证请求的 DEBUG 长度。 |
| 20–21 | 客户端分别报告上传到服务端家目录、下载到独立工作目录。 |
| 26–28 | 仓库原文件、服务端 `$HOME/client.c`、下载副本 `./client.c` 的 SHA-256 相同，也与传输前一致。 |

复核时，三份文件仍在，均为 6003 字节；重新计算的 SHA-256 与录制一致：

```text
eb6cdf34ccacc744fc549e610cb7c75df340aa938c2c79ad8abd4ffc01b57428
```

## 复现

1. 沿用 [CP2 的实验 sshd](../cp2/README.md)。先确认远端家目录下的 `client.c` 不存在（含符号链接）；如已存在，先核实来源并妥善保留，勿直接覆盖。
2. 在仓库根目录构建客户端并准备独立工作目录：

   ```sh
   cmake -S . -B build/cp6-notice \
     -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-16 \
     -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3 \
     -DCMAKE_C_FLAGS="-DMACOS -DLOG_LEVEL=NOTICE"
   cmake --build build/cp6-notice --parallel 4
   export CP6_REPO="$PWD"
   run_dir=$(mktemp -d "$CP6_REPO/build/cp6-run.XXXXXX") &&
     mkdir "$run_dir/upload" &&
     cp "$CP6_REPO/client.c" "$run_dir/upload/client.c" &&
     cd "$run_dir"
   ```

3. 运行 `/usr/bin/script "$CP6_REPO/checkpoint/cp6/typescript-rerun"`，避免覆盖原始录制。在录制会话中先运行 `shasum -a 256 "$CP6_REPO/client.c"`，再运行：

   ```sh
   stty -echo && "$CP6_REPO/build/cp6-notice/client" username@127.0.0.1; stty echo
   ```

   将 `username` 换成实验账号，等到 `password:` 后手动输入密码；录制只用默认输出模式。进入 `sftp>` 后，按提示逐行输入 `put`、`upload/client.c`、`get`、`client.c`、`bye`。下载落在新建工作目录，不会覆盖仓库原文件。

4. 仍在录制会话中运行 `shasum -a 256 "$CP6_REPO/client.c" "$HOME/client.c" ./client.c`，确认三个值与传输前相同，然后输入 `exit` 结束录制。服务端与客户端在同一台 Mac，可直接校验服务端家目录中的文件。
