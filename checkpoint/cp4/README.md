# CP4 — SSH 密码认证

**验收结果：通过。** 2026-11-18，在 macOS 本机回环地址 `127.0.0.1:22` 使用实验 OpenSSH 10.5 服务。正式附件为 [typescript](typescript) 和本说明。录制中的源码 HEAD 为 `b59138e`，当时已跟踪的本地改动只有验收清单。

录制第 33 行出现密码提示，紧接着是发出认证请求的日志，没有密码回显。第 34 行记录收到 `SSH_MSG_USERAUTH_SUCCESS`（类型 `52`），第 35 行输出 `password authentication succeeded`，构成 CP4 的通过证据。录制正常结束。提交版省略类型 `50` 认证请求的包长、填充长度和载荷长度。

## 录制步骤

保持 [CP2 的实验 sshd](../cp2/README.md) 在独立终端运行，只监听 `127.0.0.1:22`。在另一个本机终端进入项目根目录，运行：

```sh
cmake --build build --parallel 4
/usr/bin/script checkpoint/cp4/typescript-rerun /bin/sh -c '
  stty -echo || exit 1
  printf "Running: ./build/client username@127.0.0.1\n"
  exec ./build/client username@127.0.0.1
'
```

运行前将 `username` 换成实验账号，复跑时另用录制文件名。等客户端显示 `password:` 后手动输入密码并回车。使用 `script` 的默认输出模式，不加 `-k` 或 `-r`；`stty -echo` 在启动客户端前关闭回显，失败时不启动客户端。

看到认证成功后，如果到了 `sftp>`，输入 `bye` 并回车；因为回显关闭，键入的 `bye` 也不会显示。若客户端卡在后续步骤，可按 `Ctrl-C`。客户端退出即结束本次录制。
