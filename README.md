# MiniWorld ACE Bypass Loader / 迷你世界 ACE 绕过加载器

A drop-in replacement for `minigameappbase.dll` that lets `minigameapp.exe` start directly and skips ACE (AntiCheatExpert) initialization. Intended for offline play, client debugging and reverse-engineering research.

用于替换 PC 版《迷你世界》的 `minigameappbase.dll`，让 `minigameapp.exe` 可脱离启动器直接启动，并在启动时跳过 ACE（AntiCheatExpert）初始化。适用于离线游戏、客户端调试与逆向学习。

[中文说明](#中文说明) | [English](#english)

> **Status / 状态:** Verified on PC client `3.2.6.4` (channel `apiid=110`, update 2026-11-14). / 已在 PC 客户端 `3.2.6.4`（渠道 `apiid=110`，2026-11-14 更新）验证。

---

## 中文说明

### 简介

本项目提供一个用于 PC 版《迷你世界》的 `minigameappbase.dll` 替身。替换后，`minigameapp.exe` 可脱离启动器直接启动，并跳过 ACE 初始化，便于离线游戏与客户端调试。

> 本项目与迷你世界官方、ACE 官方无关。请勿用于在线作弊或商业用途。

### 背景：旧替身为何失效

社区旧版替身（2026-07 与 2026-11-06 版本）通过特征码定位引擎启动函数并直接调用，旧客户端可用。新版 `libiworld.dll` 在启动过程中会**两次**加载 `first_res.pkg`：

- 第一次加载成功并返回对象；
- 第二次发生在 `InitAceClient5`（ACE 初始化）期间/之后，接口返回 `NULL`（日志出现 `first_res already exists`）；
- 新版引擎未对该返回值判空，随后在 `libiworld+0x57EBB4` 空指针崩溃（`0xC0000005`）。

因此，仅“扫描特征码 + 调用启动函数”在新版上已经不够，还必须处理 ACE 初始化引发的重复加载。

### 工作原理

1. `LoadLibraryA("libiworld.dll")`。
2. 用唯一特征码定位 ACE 加载函数（测试版本 RVA `0x143AC50`，特征 `40 53 48 81 EC 30 08 00 00`），将其入口补丁为 `33 C0 C3`（`xor eax, eax; ret`），从而完全跳过 ACE 初始化。
3. 用唯一函数入口特征码定位引擎启动函数 `FhNxBXn1gE3W`（测试版本 RVA `0x807C40`）：
   `48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 4C 8B F1`
   若未命中，回退到旧版 AOB 扫描：`49 8B F0 48 8B EA E8 ?? ?? ?? ?? 48 8B D8 48 85 C0`，命中地址 `- 0x25`。
4. 调用 `start(0, 0, "apiid=110", 1)`，随后返回，由 `minigameapp.exe` 继续执行。

跳过 ACE 后，`first_res.pkg` 只加载一次，崩溃消失，引擎可正常初始化至主菜单。

### 兼容性与验证

- 测试客户端：PC 版 `3.2.6.4`（渠道 `apiid=110`，`zhengshibeta641601-10-29`），`libiworld.dll` 更新于 2026-11-14。
- 验证结果：
  - 直接启动 `minigameapp.exe` 后游戏窗口正常出现；
  - `GameApp.log` 到达 `MainMenuStage beginGame1` 与 `ModMgrCheckInitalFinish`；
  - 无 `ACE-*` 进程；
  - 进程稳定运行。
- 游戏更新后特征码可能变化；若启动失败，请提交客户端版本、`libiworld.dll` 版本与日志。

### 构建

环境要求：Windows x64 + Visual Studio 2022/2026（C++ 桌面开发工作负载）。

在仓库根目录运行：

```bat
build.bat
```

或在 “x64 Native Tools Command Prompt” 中手动编译：

```bat
cl /nologo /LD /O2 /MT /GS- /W3 src\minigameappbase.c /link /DEF:src\minigameappbase.def /OUT:dist\minigameappbase.dll
```

输出文件：`dist\minigameappbase.dll`。

### 安装与使用

1. 关闭游戏。
2. 备份原文件：将 `<安装目录>\miniworldLauncher\minigameappbase.dll` 复制为 `minigameappbase.dll.orig`。
3. 将 `dist\minigameappbase.dll` 覆盖到同一目录。
4. 直接运行 `minigameapp.exe`（桌面“启动游戏”快捷方式）。
5. **不要通过启动器启动**：启动器可能校验或自动修复 DLL，从而覆盖本文件。

### 卸载

将备份的 `minigameappbase.dll.orig` 重命名回 `minigameappbase.dll` 即可。

### 限制与注意事项

- **不初始化 ACE**：联机、登录、排行榜等依赖 ACE 的功能可能不可用；仅建议离线/调试使用。
- 启动器可能还原官方 DLL；游戏更新可能改变特征码。
- `apiid=110` 为当前渠道的硬编码参数；其他渠道请自行修改源码。
- 游戏本体要求管理员权限，本 DLL 不额外提权。
- DLL 未签名，可能触发 SmartScreen 或杀毒软件误报。

### 故障排查

| 现象 | 可能原因 | 处理方式 |
| --- | --- | --- |
| 退出码 `0xDEADC0DE` | 仍在使用官方 DLL，或缺少启动器上下文 | 确认替身 DLL 已覆盖，并直接从 exe 启动 |
| 崩溃在 `libiworld+0x57EBB4` | ACE 跳过补丁未生效（特征码未命中） | 检查游戏版本；更新特征码或提交 issue |
| 启动器启动后失效 | 启动器自动修复并覆盖 DLL | 改用直接启动，或再次覆盖 |
| 无窗口 / 黑屏 | 启动失败或资源问题 | 查看 `GameApp.log` 与系统事件日志 |

### 目录结构

```text
Pass Miniworld ACE/
├─ README.md
├─ build.bat
├─ src/
│  ├─ minigameappbase.c
│  └─ minigameappbase.def
└─ dist/
   └─ minigameappbase.dll
```

### 预编译文件

`dist/minigameappbase.dll`（v1.60.1，2026-11-14）

也可从 [GitHub Releases](https://github.com/Daling713/Make-Mini-World-have-no-ACE/releases/latest) 下载。

- SHA-256: `160F31AA2F055B7EF239F9DB80E17C22D97FBFB8286D64D292D79089381ACEE6`
- MD5: `8561A9FAAB38EDC3ACD3C2B3CCD3E28D`

### 致谢

- 引擎启动函数的定位与调用思路参考了公开的社区“过 ACE”分析；实现、补丁、文档均为原创。
- 感谢 IDA Pro、x64dbg、Cheat Engine 等调试与分析工具。

### 免责声明

本项目仅供学习、研究与离线调试使用。使用本工具可能违反游戏服务条款，可能导致账号封禁；请自行承担风险。作者不对任何直接或间接损失负责。禁止用于在线作弊、商业用途或侵犯他人权益。

### 许可证

待定（建议 MIT）。正式发布前请补充 `LICENSE` 文件。

---

## English

### Introduction

This repository provides a replacement `minigameappbase.dll` for the PC version of *MiniWorld*. Once installed, `minigameapp.exe` can be started directly without the launcher, with ACE (AntiCheatExpert) initialization skipped. It is intended for offline play, client debugging and reverse-engineering research.

> This project is not affiliated with MiniWorld or ACE. Do not use it for online cheating or commercial purposes.

### Background: Why older bypass DLLs stopped working

Older community bypass DLLs (the 2026-07 and 2026-11-06 builds) located the engine start function by signature and called it directly. That worked on older clients. In the newer `libiworld.dll`, `first_res.pkg` is loaded **twice** during startup:

- The first load succeeds and returns an object.
- The second load happens around `InitAceClient5` (ACE initialization) and returns `NULL` (the log shows `first_res already exists`).
- The new engine never checks that return value and dereferences `NULL`, crashing at `libiworld+0x57EBB4` (`0xC0000005`).

Therefore “scan for the signature and call the start function” is no longer sufficient; the duplicate load caused by ACE initialization must be prevented.

### How it works

1. `LoadLibraryA("libiworld.dll")`.
2. Locate the ACE loader function by a unique signature (RVA `0x143AC50` in the tested build; signature `40 53 48 81 EC 30 08 00 00`) and patch its entry to `33 C0 C3` (`xor eax, eax; ret`), completely skipping ACE initialization.
3. Locate the engine start function `FhNxBXn1gE3W` (RVA `0x807C40` in the tested build) by its unique prologue signature:
   `48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 4C 8B F1`
   If that fails, fall back to the legacy AOB scan `49 8B F0 48 8B EA E8 ?? ?? ?? ?? 48 8B D8 48 85 C0` and subtract `0x25` from the match address.
4. Call `start(0, 0, "apiid=110", 1)` and return; `minigameapp.exe` continues execution.

With ACE skipped, `first_res.pkg` is loaded only once, the crash disappears, and the engine reaches the main menu.

### Compatibility & verification

- Tested client: PC `3.2.6.4` (channel `apiid=110`, `zhengshibeta641601-10-29`), `libiworld.dll` updated on 2026-11-14.
- Verified:
  - The game window appears after starting `minigameapp.exe` directly.
  - `GameApp.log` reaches `MainMenuStage beginGame1` and `ModMgrCheckInitalFinish`.
  - No `ACE-*` processes are created.
  - The process stays stable.
- Signatures may change after a game update. If startup fails, please include the client version, `libiworld.dll` version and logs in an issue.

### Build

Requirements: Windows x64 and Visual Studio 2022/2026 with the C++ desktop development workload.

From the repository root:

```bat
build.bat
```

Or build manually from an “x64 Native Tools Command Prompt”:

```bat
cl /nologo /LD /O2 /MT /GS- /W3 src\minigameappbase.c /link /DEF:src\minigameappbase.def /OUT:dist\minigameappbase.dll
```

Output: `dist\minigameappbase.dll`.

### Installation & usage

1. Close the game.
2. Back up the original: copy `<install dir>\miniworldLauncher\minigameappbase.dll` to `minigameappbase.dll.orig`.
3. Copy `dist\minigameappbase.dll` over the file in the same directory.
4. Run `minigameapp.exe` directly (the desktop “Launch Game” shortcut).
5. **Do not launch through the launcher**: it may verify or automatically repair the DLL and overwrite this file.

### Uninstall

Rename the backup `minigameappbase.dll.orig` back to `minigameappbase.dll`.

### Limitations & notes

- **ACE is not initialized**: online play, login and leaderboard features that depend on ACE may not work. Offline/debug use only.
- The launcher may restore the official DLL; game updates may change the signatures.
- `apiid=110` is hardcoded for the current channel; adjust the source if you use another channel.
- The game itself requires administrator privileges; this DLL does not request additional elevation.
- The DLL is unsigned and may trigger SmartScreen or antivirus false positives.

### Troubleshooting

| Symptom | Possible cause | Fix |
| --- | --- | --- |
| Exit code `0xDEADC0DE` | Official DLL still in use, or launcher context missing | Make sure the replacement DLL is installed and start the exe directly |
| Crash at `libiworld+0x57EBB4` | ACE-skip patch not applied (signature not found) | Check the game version; update signatures or open an issue |
| Works once, then stops working | Launcher repaired/overwrote the DLL | Start directly or reinstall the replacement |
| No window / black screen | Startup failure or resource issue | Check `GameApp.log` and Windows event logs |

### Repository layout

```text
Pass Miniworld ACE/
├─ README.md
├─ build.bat
├─ src/
│  ├─ minigameappbase.c
│  └─ minigameappbase.def
└─ dist/
   └─ minigameappbase.dll
```

### Prebuilt binary

`dist/minigameappbase.dll` (v1.60.1, 2026-11-14)

You can also download it from [GitHub Releases](https://github.com/Daling713/Make-Mini-World-have-no-ACE/releases/latest).

- SHA-256: `160F31AA2F055B7EF239F9DB80E17C22D97FBFB8286D64D292D79089381ACEE6`
- MD5: `8561A9FAAB38EDC3ACD3C2B3CCD3E28D`

### Credits

- The approach for locating and calling the engine start function was informed by publicly shared community “ACE bypass” analysis. The implementation, patch and documentation are original.
- Thanks to IDA Pro, x64dbg and Cheat Engine for analysis and debugging.

### Disclaimer

This project is provided for educational, research and offline debugging purposes only. Using it may violate the game's terms of service and may result in an account ban. Use at your own risk. The author is not responsible for any direct or indirect damages. Online cheating, commercial use and infringement of third-party rights are prohibited.

### License

TBD (MIT recommended). Add a `LICENSE` file before publishing.
