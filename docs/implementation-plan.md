# Lab 4 实现计划与框架审查

审查日期：2026-11-16（北京时间）。上游：[SOAR-PKU/lab-sftp](https://github.com/SOAR-PKU/lab-sftp)。
基线提交：`23eebe212b11241c866fee33794b998e25223630`。以下行号针对该基线，后续以函数名定位。

目标：两人补全最小 SFTP 客户端，与真实 SSH 服务器完成 `client.c` 上传和下载，收集 CP1–CP6 证据。课程课件截止时间为 **2026-11-23 23:59:59（UTC+8）**。

本次只做框架审查、实现计划和协作初始化；尚未实现任何 PT、未连接 SSH 服务器、未获得 CP 验收结果。

## 1. 先解决算法要求与源码不一致

| 项目 | 2026 课件 / 上游 README | 当前源码 |
| --- | --- | --- |
| 密钥交换 | `diffie-hellman-group1-sha1` | `diffie-hellman-group14-sha256` |
| 对称加密 | `aes256-cbc` | `aes256-ctr` |
| 完整性校验 | `hmac-sha1` | `hmac-sha1` |
| Host key | README 描述 `ssh-rsa` | 协商列表为 `ssh-rsa` |

证据：`src/kex.c:25` 的 `supported_methods`；`ssh_select_kex()` 设置 group14 枚举；`src/dh.c:105` 初始化 group14 参数，`dh_compute_session_id()` 使用 SHA-256。底层加密表虽然包含 CBC 和 CTR，但 DH 部分不能只改一个算法名。

- [ ] 两人先向助教确认提交应遵循当前框架还是课件旧套件，记录答复；本次未代为联系。
- [ ] 答复前以当前源码套件作为开发基线，接口与通用逻辑可先推进，不能宣称已符合课件算法要求。
- [ ] 若必须支持旧套件，另开任务一致修改 DH 群参数、交换哈希、KDF 类型、协商枚举和 cipher，并重复 CP2–CP6。
- [ ] 测试服务器采用经确认的套件配置；仅在实验服务实例中调整兼容选项，不改其他服务的全局配置。

## 2. 代码框架与调用顺序

```text
client.c: main
  ssh_new / ssh_options_set
  ssh_connect                       src/session.c
    ssh_socket_connect              src/socket.c -> 系统 TCP socket
    send_id_str / receive_id_str     版本交换
    ssh_set_client_kex / ssh_send_kex / ssh_receive_kex / ssh_select_kex
                                    src/kex.c -> 算法协商
    ssh_dh_handshake                src/dh.c -> DH、K/H、KDF、NEWKEYS
    ssh_request_auth                src/auth.c -> ssh-userauth service
  ssh_userauth_password             src/auth.c -> 用户认证
  sftp_new                          src/sftp.c
    ssh_channel_new / ssh_channel_open_session / ssh_channel_request_sftp
                                    src/channel.c
  sftp_init                         SFTP v3 初始化
  get / put                         client.c 交互入口
    sftp_open / sftp_read|write / sftp_close
      sftp_packet_read|write
        ssh_channel_read|write
          ssh_packet_receive|send   src/packet.c -> 加解密、MAC、SSH framing
            ssh_socket_read|write
  sftp_free / ssh_free
```

已提供的基础设施：`buffer.c`（编码/解码）、`string.c`（SSH 长度前缀字符串）、`bignum.c`、`libcrypto.c`、`crypto.c`、`kdf.c`（加密封装与派生）、错误和日志工具。先复用，再针对实际问题修复。

`client.c` 已有用户名/主机参数、密码循环、get/put/bye；不需要重写一个完整命令行客户端。框架每个 SSH session 仅支持一个 channel，目录操作、多通道和 rekey 不列入本次必做扩展。

## 3. 全部明确填空：28 处

可复核：`rg -n 'LAB:' src`。搜索 TODO 不够，因为主要缺口标记为 `LAB`。

| 文件 | 基线行号 | 数量 | 必须补全的内容 | 对应阶段 |
| --- | --- | ---: | --- | --- |
| `src/session.c` | 199 | 1 | `receive_id_str()` 处理前置文本行、版本行、CRLF、长度与读错误，保存服务端版本 | PT1 / CP1 |
| `src/kex.c` | 187、232 | 2 | 解码 server name-lists，保留 exchange hash 的原始消息；按客户端偏好选交集，处理空语言列表与不兼容算法 | PT2 / CP2 |
| `src/dh.c` | 601、638 | 2 | 调用已有工具计算共享秘密 K、交换哈希 H；设置 `current_crypto` / 方向使用状态、完成 NEWKEYS 切换 | PT3 / CP3 |
| `src/packet.c` | 282 | 1 | 对解密后的完整 SSH packet 验证 MAC，再交付 payload | PT3 支撑 |
| `src/auth.c` | 125、128、132、135 | 4 | banner、成功、认证失败/密码修改请求、其他消息的处理，明确重试和致命错误 | PT4 / CP4 |
| `src/channel.c` | 102、105、125、128 | 4 | channel open 成功/失败、global request、其他消息，记录双方编号与窗口 | PT5 / CP5 |
| `src/channel.c` | 491、511、514、517、520、523、526 | 7 | 缓冲数据读取、WINDOW_ADJUST、DATA、EOF、CLOSE、CHANNEL_REQUEST、其他消息 | PT5–PT6 共用 |
| `src/sftp.c` | 239、251、254、257 | 4 | OPEN 请求编码及 STATUS/HANDLE/异常响应 | PT6 / CP6 |
| `src/sftp.c` | 304、355、405 | 3 | CLOSE / READ / WRITE 响应分支、ID、状态、偏移、EOF、资源释放 | PT6 / CP6 |

细节：

- `dh_compute_shared_secret()`、`dh_compute_session_id()`、`dh_gen_session_keys()` 已存在，缺的是连接这些步骤和正确启用加密，不是从零实现密码算法。
- NEWKEYS 本身使用旧密钥；发出/收到 NEWKEYS 后分别切换对应方向，不能把 NEWKEYS 用新密钥加密，也不能重置 SSH 包序号。
- MAC 使用序号和包含长度/填充的未加密 packet，不能只对 payload 验证；先验 MAC 再去除 framing。
- 认证失败可以向 `client.c` 返回 `SSH_AGAIN` 以重输密码；不支持的密码修改流程应明确报错，不能伪装认证成功。
- `sftp_write()` 要在成功 STATUS 后推进 `nleft` 和文件偏移，否则已有循环无法终止；`sftp_read()` 按实际字节数更新偏移。
- `sftp_stat()` 是额外空桩，课程限定 OPEN/CLOSE/READ/WRITE，当前不纳入必做范围。数量 28 不包含这个扩展空桩。

## 4. 填空之外必须关注的现有问题

这些是静态代码观察；除编译外未通过运行复现，需在实施时验证。

| 优先级 | 位置 | 已观察到的逻辑 | 计划处理 / 验证 |
| --- | --- | --- | --- |
| 高 | `socket.c` | `ssh_socket_write()` 只有一次 write；read 返回 0 不退出；connect 循环全部失败后仍可能返回成功 | 完整写、EINTR、EOF 和连接失败必须返回；断连测试不能挂死 |
| 高 | `packet.c` | 多处读返回值未检查；由网络 packet_length 直接计算读长与分配长度 | 在分配前验证上下界、溢出、对齐、padding；拒绝截断报文 |
| 高 | `channel.c` | `ssh_channel_read()` 使用函数级 static 缓冲，错误释放后没有清空 static 指针 | 建议改为 channel 所有，约定 EOF 前先交付缓存，避免会话串数据或重复释放 |
| 高 | `sftp.c` | close/read/write 部分失败分支会再次释放此前已释放的 buffer | 整理单一路径清理或释放后置 NULL，检查失败路径 |
| 高 | `sftp.c` | header 读取遇到 SSH_EOF 仍可能继续解析；长度减 1、分配与边界顺序有风险 | 要求完整 header，分配前校验长度，截断/异常长度应失败 |
| 中 | `channel.c` | wait_window/channel_request 等路径支持的穿插消息有限；remote_maxpacket 减 10 未先检查下限 | 有效分派 global/channel 消息；测试小窗口和过小 maxpacket，不丢数据 |
| 中 | `session.c` / `socket.c` | free 函数未完整释放结构和已激活 crypto 等资源 | 成功/失败出口检查所有权；避免 fd 双关或 fd 0 误处理 |
| 中 | `client.c` | get 在当前目录按文件 basename 创建并截断；put 在本地 open 前打开远端文件 | 验收下载到独立目录；检查本地文件存在后再创建远端目标；检查交互 EOF 与短写 |

不要在第一阶段大规模重构；优先修复会阻塞真实互通、导致死循环/内存错误的问题。每个修复附对应失败场景。

## 5. 两人分工与接口契约

建议 **Semt0 负责 A，VVKAHPM 负责 B**；这是可交换的建议，尚不代表双方认领。

| 工作流 | 主文件 | 交付边界 |
| --- | --- | --- |
| A：传输与认证 | session/socket/packet/kex/dh/auth | 真实 SSH 登录成功；向上提供完整、已验 MAC 的 SSH payload；CP1–CP4 |
| B：通道与文件 | channel/sftp/client | channel 数据流、SFTP v3 文件操作；CP5–CP6；先用模拟输入输出独立推进 |
| 共同 | 公共头文件、buffer/string/crypto 工具、构建、测试材料 | 接口变更和最终验收需交叉检查 |

**开始并行前先确认以下目标契约。它们含拟修正行为，不是对现状正确性的声明。**

1. `ssh_packet_send()`：调用方填 `session->out_buffer`，成功返回 SSH_OK 并清空发送缓冲；失败不宣称发送完成。接收成功时 `in_buffer` 只含一个完整 payload（含消息类型），下一次 receive 会覆盖，调用方不能保留裸指针。
2. socket 层内部负责短读/短写；read 要么读满所需长度，要么明确 EOF/错误，不能无限等待。write 保持成功返回写入字节数的现有调用约定，所有调用方一致检查。
3. `ssh_channel_read()` 保持框架“读满 count”的用途。若 EOF 前还有数据先返回实际字节数，耗尽后返回 SSH_EOF；SFTP 层据此拒绝截断 header/body。channel 写成功返回全部写入字节数，错误返回 SSH_ERROR。
4. `sftp_read()` 文件 EOF 返回 0，错误返回负值；`sftp_write()` 成功返回确认写入的字节数。SSH channel EOF 与 SFTP 文件 EOF 不混用。
5. 状态/控制函数一般返回 SSH_OK / SSH_ERROR；认证的 SSH_AGAIN 专用于可重试失败，不能当“其他负数都一样”。
6. 发送使用对端 channel ID，接收核对本端 ID。window 按 channel data 字节记账；SFTP 报文可跨多个 CHANNEL_DATA，不能把 TCP/SSH/channel/SFTP 四种边界等同。
7. SFTP response 必须匹配 request ID，read 数据长度不得超过请求；handle 属于 file，response/status 临时对象由当前操作清理；close 的所有权行为统一写清。
8. packet 层只做 framing/crypto；上层负责按消息类型分派。IGNORE/DEBUG/DISCONNECT/global request 等通用消息的处理由双方约定，避免多个层各自读取并丢包。

## 6. 实施顺序与建议日程

| 日期（北京时间） | A 工作 | B 工作 | 阶段出口 |
| --- | --- | --- | --- |
| 11/16 | 确认算法问题、构建环境、socket 契约 | 阅读 channel/SFTP，约定接口与证据格式 | 双方能编译；实验服务器方案明确 |
| 11/17 | 版本交换与算法协商 | channel open/request、消息解析模拟测试 | CP1–CP2；channel 接口可评审 |
| 11/18 | DH、密钥派生接线、NEWKEYS、MAC | channel 流读取、窗口、EOF；SFTP OPEN/CLOSE | 加密往返打通，收集 CP3 |
| 11/19 | 密码认证、失败重试、支持上层联调 | SFTP READ/WRITE、ID/状态/偏移处理 | CP4；完成首次 CP5 联调 |
| 11/20 | 互通问题定位与底层失败路径 | 完成真实上传/下载和内容校验 | CP6 首次真实通过 |
| 11/21–22 | 交叉 review、异常输入/断连测试 | 空文件/多块/大文件、小窗口、证据整理 | 逐项复验 CP1–CP6，冻结提交候选 |
| 11/23 | 共同从干净 checkout 重建、复跑、归档检查 | 同左 | 截止前提交，留出缓冲时间 |

关键依赖：PT1 → PT2 → PT3 → PT4 → PT5 → PT6。B 可先做结构和模拟测试，但真实 CP5/6 需要 A 的加密与认证通路。不要等到所有填空结束才首次连接服务器。

## 7. 验证计划

- 基础 IO：服务端关闭、半包到达、短写/EINTR、连接失败；有超时的测试必须及时退出。
- 协商：前置文本行、空语言列表、算法无交集、KEX 消息保真；确认实际选中的套件。
- 传输：明文包和加密包往返、错误 MAC 拒绝、异常长度/填充拒绝、NEWKEYS 边界、双向序号；辅助日志不得泄露密码和密钥。
- 认证：正确密码、错误密码后重试、banner、服务器拒绝方法或断开。
- channel：open/subsystem 成功与失败；global request 的 want-reply；数据跨包与同包多次读取；WINDOW_ADJUST、EOF/CLOSE、小窗口发送。
- SFTP：不存在文件、权限拒绝、请求 ID 不符、短读、空文件、跨 32768 字节多块文件；WRITE 成功才推进偏移，EOF 结束而不丢最后一块。
- 最终互通：真实 SSH 服务端上传/下载 `client.c`，再测试空文件和多块二进制文件；对比原文件、远端、下载副本的长度和 SHA-256。
- 内存检查：用可用的 AddressSanitizer 等检查异常退出、重复 open/close 和错误响应路径；测试构建与常规构建分开。

正式 CP 材料见 [checkpoint/README.md](../checkpoint/README.md)。每次记录服务器版本、实际算法、提交号和操作步骤。编译成功、模拟测试通过、真实互通通过是不同证据，不互相替代。

## 8. 本次基线构建结果

2026-11-16 macOS 本机验证：CMake 配置成功，GCC 16.2.0 + OpenSSL 3.6.4 成功构建共享库 `sftp` 与 `client`。OpenSSL 3 的 HMAC 旧接口产生弃用警告，没有构建错误。没有执行联网客户端或任何功能测试。

```sh
cmake -S . -B build \
  -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3 \
  -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-16 \
  -DCMAKE_C_FLAGS="-DMACOS -DLOG_LEVEL=DEBUG"
cmake --build build --parallel 4
```

上面是本机路径，其他 macOS 机器需使用自己的 GCC/OpenSSL 路径。Linux 配置：

```sh
cmake -S . -B build -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_C_FLAGS="-DLINUX -DLOG_LEVEL=DEBUG"
cmake --build build --parallel 2
```

原 `.github/workflows/main.yaml` 是课程维护者的发布流程，引用缺失的 `publish/main.sh` 并指向 SOAR-PKU 仓库。本协作仓库移除该发布流程，并将新的 Linux 编译检查配置保存在 `docs/build-workflow.yml`。当前 GitHub 登录凭据缺少 workflow 权限，SSH 推送也未成功，因此自动检查尚未启用；获得相应权限后移至 `.github/workflows/build.yml` 即可启用。该配置不包含协议功能测试，Linux 云端编译尚未验证。

## 9. 提交准备

- [ ] 全部 PT/CP 按实际完成程度核对；保留可复现证据。
- [ ] `not-implemented.pdf` 按真实未完成项和原因生成，不能提前标记“全部完成”。
- [ ] 两人提交包命名：`lab4-name1-studentID1-name2-studentID2.zip`（或 tar/tar.gz）；姓名学号待双方填写。
- [ ] 整个工程、checkpoint 和必需说明入包，排除本地构建缓存与秘密信息。
- [ ] 从干净解压目录编译并复跑 CP6，再按课程渠道提交；上游 README 当前指定邮箱 `liuxym@pku.edu.cn`。

来源：上游 README 与本基线源码；截止时间和 AI 协作要求来自本地 `Lab4_2026.pdf`（课件第 1、3、11 页）。课程要求学生自己理解并实现项目，本计划用于组织学习、实现和验证。
