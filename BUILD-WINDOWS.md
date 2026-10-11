# Windows 构建与测试说明（MinGW-w64）

本文件记录在 Windows（MinGW-w64 / gcc 13.1，x86_64-posix-seh）上从源码构建 sqlean 扩展并跑通测试套件的完整过程。上游 `Makefile` 的 `compile-windows` 目标可用，但有两处 Windows 兼容问题需要处理（见下）。

## 1. 前置依赖

- 编译器：MinGW-w64 gcc/g++/mingw32-make（x86_64-posix-seh，如 Qt 自带的 `Tools/mingw1310_64`）。
- SQLite amalgamation：`src/` 下**不包含** `sqlite3.c / sqlite3.h / sqlite3ext.h / shell.c`，需要先获取。
  上游 `download-sqlite` 目标写死的是 2021 年版 `sqlite-amalgamation-3360000.zip`，从 https://sqlite.org/2021/sqlite-amalgamation-3360000.zip 下载并解压，把 `sqlite3.c / sqlite3.h / sqlite3ext.h / shell.c` 放到 `src/`。
- `test_windirent.h`：`src/fileio/legacy.c` 会包含它，从 mackyle/sqlite 的 `branch-3.36` 获取，放到 `src/`。

## 2. 编译

```powershell
# PATH 加入 MinGW 工具链
$env:PATH = "D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
mingw32-make compile-windows
```

产物在 `dist/`：13 个扩展 DLL（crypto / define / fileio / fuzzy / ipaddr / math / regexp / stats / text / time / unicode / uuid / vsv）外加全合一的 `sqlean.dll`。

## 3. 测试

### 3.1 准备 64 位 sqlite3 命令行

官方 sqlite.org 提供的 `sqlite-tools-win32-x86` 是 **32 位**，无法加载 64 位扩展 DLL（报错"%1 不是有效的 Win32 应用程序"）。需要自编一个 64 位 shell：

```powershell
gcc -O2 -Isrc src/sqlite3.c src/shell.c -o bin/sqlite3-x64.exe -lm
```

### 3.2 运行时依赖（重要）

MinGW posix 线程模型下，`-shared` 编译的 DLL 会把 `libwinpthread-1.dll` 记录为导入依赖（即使 DLL 里没有任何 pthread 符号）。`dist/time.dll`、`dist/uuid.dll`、`dist/sqlean.dll` 都有此依赖。

把 `libwinpthread-1.dll`（来自 MinGW `bin/`）**复制到 sqlite3.exe 同目录**（即 `bin/`），否则 `.load dist/uuid` 会报"找不到指定的模块"。

### 3.3 跑测试

测试 SQL 用相对路径 `.load dist/<name>`，需要在仓库根目录执行，并用 cmd 字节重定向（避免 PowerShell 管道破坏 UTF-8）：

```powershell
cmd /c "bin\sqlite3-x64.exe < test\crypto.sql > test\crypto.log 2>nul"
```

## 4. 本地测试结果（gcc 13.1 x86_64，SQLite 3.36.0）

| 套件 | 用例数 | 通过 |
|---|---|---|
| crypto | 169 | 169 |
| define | 43 | 43 |
| fuzzy | 58 | 58 |
| ipaddr | 29 | 29 |
| math | 61 | 61 |
| regexp | 37 | 37 |
| stats | 21 | 21 |
| text | 450 | 450 |
| time | 353 | 353 |
| unicode | 14 | 14 |
| uuid | 33 | 33 |
| sqlean | 24 | 24 |
| fileio | 39 | 21 |
| vsv | 3 | 2 |

合计 **1334 用例，19 失败，通过率 98.6%**。

失败的 19 例全部来自测试脚本的 Unix shell 依赖（`fileio` 的 `.system rm/touch/printf`、`vsv` 的 `.shell echo '...' > file.csv` 单引号重定向语法），Windows cmd 不识别这些命令，与扩展本身的实现无关。

## 5. 对上游 Makefile 的 Windows 兼容修复

### 5.1 两级通配符不展开（`src/text/*/*.c`）

MinGW 的 gcc 在 Windows 下不展开 shell 风格的两级 glob，`compile-windows` 原命令 `src/text/*.c src/text/*/*.c` 会报 `cc1.exe: fatal error: src/text/*/*.c: Invalid argument`。本仓库 `src/text/` 下只有 `utf8/` 一个子目录，已显式改为 `src/text/utf8/*.c`（Windows 两条编译规则，其他平台不动）。

### 5.2 缺少 dist 目录

`compile-windows` 直接写 `dist/*.dll`，但 `dist/` 不存在时 `ld` 报 `cannot open output file dist/crypto.dll: No such file or directory`。构建前先 `mkdir dist`。
