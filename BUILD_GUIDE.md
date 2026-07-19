# LeviLamina Mod 构建配置指南

> 基于 levilamina `v26.10.14` 在 Windows x64 + GitHub Actions 上的实战构建经验。
> 本文记录了从模板克隆到 CI 成功产出 artifact 过程中遇到的所有上游坑及最终可用的配置。

---

## 1. 环境要求

| 组件 | 版本 |
|------|------|
| OS | Windows 10/11 x64（CI 用 `windows-latest`） |
| xmake | ≥ 3.0（CI 用 `xmake-io/github-action-setup-xmake@v1`） |
| 编译器 | **MSVC**（VS 2022+，随 `windows-latest` 提供） |
| C++ 标准 | C++20 |
| Runtime | MD（动态 CRT） |
| levilamina | 26.10.14（锁版本） |
| levibuildscript | latest |

---

## 2. xmake.lua 关键配置

### 2.1 锁定 levilamina 版本

```lua
add_requires("levilamina 26.10.14", {configs = {target_type = get_config("target_type")}})
add_requires("levibuildscript")
```

> 不要用不锁版本的 `add_requires("levilamina")`，上游包配方变化会破坏构建。

### 2.2 **必须使用 MSVC，不能用 clang-cl**

模板默认 `set_toolchains("clang-cl")`，**必须改为**：

```lua
if is_plat("windows") then
    add_defines("NOMINMAX", "UNICODE")
    set_exceptions("none")
    add_cxflags("/EHa", "/utf-8", "/W4", ...)
    add_cxflags("/EHs", "-Wno-...", {tools = {"clang_cl"}})  -- 保留，clang-cl 时生效
    set_toolchains("msvc")  -- ★ 关键：改用 MSVC
end
```

**原因**：levilamina 依赖 `rapidjson v1.1.0`，其 `document.h(319)` 在新版 clang-cl 下触发：
```
error: cannot assign to non-static data member 'length' with const-qualified type 'const SizeType'
```
MSVC 对此兼容，levilamina 官方也用 MSVC 构建。

### 2.3 libssh2 / libcurl 配置（可保留但 CI 中无效）

```lua
add_requireconfs("**.libssh2", {build = true})
add_requireconfs("**.libcurl", {configs = {libssh2 = true, zlib = true}})
```

> ⚠️ 注意：`add_requireconfs` 对 levilamina 包**内部**的依赖（如 cpr→libcurl→libssh2）**无法生效**，因为包安装时独立解析依赖。真正解决问题靠 CI 中的 patch 脚本（见下文）。这两行可保留作为文档说明，也可删除。

### 2.4 完整 xmake.lua 模板

```lua
add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

add_requires("levilamina 26.10.14", {configs = {target_type = get_config("target_type")}})
add_requires("levibuildscript")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("my-mod")  -- 改成你的 mod 名
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    if is_plat("windows") then
        add_defines("NOMINMAX", "UNICODE")
        set_exceptions("none")
        add_cxflags("/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
        add_cxflags(
            "/EHs",
            "-Wno-microsoft-cast", "-Wno-invalid-offsetof", "-Wno-c++2b-extensions",
            "-Wno-microsoft-include", "-Wno-overloaded-virtual", "-Wno-ignored-qualifiers",
            "-Wno-missing-field-initializers", "-Wno-potentially-evaluated-expression",
            "-Wno-pragma-system-header-outside-header",
            {tools = {"clang_cl"}}
        )
        set_toolchains("msvc")
    end
    add_packages("levilamina")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")
```

---

## 3. GitHub Actions 配置（build.yml）

### 3.1 核心思路

levilamina 依赖链：`levilamina → cpr[ssl=y] → libcurl → libssh2 → zlib`

上游 bug：`libssh2 1.11.1` 的**预编译包**是用 `zlib v1.3.1` 构建的，其 `libssh2-config*.cmake` 硬编码了 `zlib v1.3.1` 的安装路径。但 xmake-repo 当前只提供 `zlib v1.3.2`（v1.3.1 已被移除）。当 libcurl 通过 `find_package(Libssh2)` 加载该 cmake config 时，生成的 `build.ninja` 引用了不存在的 `zlib v1.3.1/.../zlib.lib`，导致：
```
ninja: error: 'C:/.../zlib/v1.3.1/.../lib/zlib.lib', needed by 'src/curl.exe', missing
```

**解决思路**（三步法）：
1. 首次 `xmake f`（容错）— 让 xmake 安装好 zlib v1.3.2 和 libssh2 1.11.1，在 libcurl 配置阶段失败
2. Patch 脚本 — 将 libssh2 cmake 目录下所有 `.cmake` 文件中 `zlib/v1.3.1/.../zlib.lib` 路径替换为当前 `zlib/v1.3.2/.../zlib.lib` 实际路径
3. 重试 `xmake f` — libcurl 这次能找到正确的 zlib，配置通过

### 3.2 完整 build.yml

```yaml
on:
  pull_request:
  push:
  workflow_dispatch:

jobs:
  build:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v7

      - uses: xmake-io/github-action-setup-xmake@v1

      - uses: actions/cache@v6
        with:
          path: |
            ~/AppData/Local/.xmake
          key: xmake-${{ hashFiles('xmake.lua') }}
          restore-keys: |
            xmake-

      - run: |
          xmake repo -u

      - name: First xmake configure (expected to fail at libcurl)
        shell: pwsh
        continue-on-error: true
        run: |
          xmake f -a x64 -m release -p windows -v --target_type=server -y

      - name: Patch libssh2 cmake config stale zlib path
        shell: pwsh
        run: |
          # 查找当前 zlib v1.3.2 的实际安装路径（xmake f 已安装 zlib）
          $zlibLib = Get-ChildItem "$env:LOCALAPPDATA\.xmake\packages\z\zlib\v1.3.2\*\lib\zlib.lib" -ErrorAction SilentlyContinue | Select-Object -First 1
          if (-not $zlibLib) {
            Write-Error "zlib v1.3.2 not found"; exit 1
          }
          $newZlibPath = $zlibLib.FullName -replace '\\','/'
          Write-Host "Current zlib v1.3.2 path: $newZlibPath"

          # 查找 libssh2 cmake config 目录下所有 .cmake 文件（路径含 hash，用通配符）
          $cmakeDir = Get-ChildItem "$env:LOCALAPPDATA\.xmake\packages\l\libssh2\1.11.1\*\lib\cmake\libssh2" -ErrorAction SilentlyContinue | Select-Object -First 1
          if (-not $cmakeDir) {
            Write-Host "libssh2 cmake dir not found, nothing to patch"
          } else {
            Write-Host "libssh2 cmake dir: $($cmakeDir.FullName)"
            $cmakeFiles = Get-ChildItem -Path $cmakeDir.FullName -Filter "*.cmake" -Recurse
            foreach ($cfg in $cmakeFiles) {
              $content = Get-Content $cfg.FullName -Raw
              # 替换所有 zlib v1.3.1 的路径引用为当前 v1.3.2 实际路径
              $patched = $content -replace '[A-Za-z]:[/\\][^"]*zlib[/\\]v1\.3\.1[/\\][^"/\\]+[/\\]lib[/\\]zlib\.lib', $newZlibPath
              if ($patched -ne $content) {
                Set-Content -Path $cfg.FullName -Value $patched -NoNewline
                Write-Host "Patched $($cfg.Name) -> zlib path: $newZlibPath"
              }
            }
          }

      - name: Retry xmake configure after patch
        shell: pwsh
        run: |
          xmake f -a x64 -m release -p windows -v --target_type=server -y

      - run: |
          xmake -v -y

      - uses: actions/upload-artifact@v7
        with:
          name: ${{ github.event.repository.name }}-${{ github.sha }}-server-windows-x64
          path: |
            bin/
```

### 3.3 关键点说明

| 配置项 | 说明 |
|--------|------|
| `continue-on-error: true` | 首次 `xmake f` 必须容错，它会在 libcurl 配置阶段失败，但 zlib 和 libssh2 已安装 |
| `actions/cache@v6` | 缓存 `~/AppData/Local/.xmake`，key 用 `xmake.lua` 哈希。xmake.lua 改动会自动失效重建 |
| Patch 正则 | `[A-Za-z]:[/\\][^"]*zlib[/\\]v1\.3\.1[/\\][^"/\\]+[/\\]lib[/\\]zlib\.lib` — 匹配形如 `C:/.../zlib/v1.3.1/<hash>/lib/zlib.lib` 的路径 |
| `Get-ChildItem ... -Recurse` | libssh2 的 cmake config 可能有多个文件（`libssh2-config.cmake`、`libssh2-config-release.cmake`），必须全部处理 |
| artifact 命名 | `${{ github.event.repository.name }}-${{ github.sha }}-server-windows-x64` |

---

## 4. 本地构建

本地构建同样会遇到 libssh2/zlib 问题，可手动执行 patch：

```powershell
cd my_mod
xmake repo -u

# 首次配置（会失败）
xmake f -a x64 -m release -p windows --target_type=server -y

# 手动 patch libssh2 cmake config（参考 build.yml 中的脚本）
# 将 %LOCALAPPDATA%\.xmake\packages\l\libssh2\1.11.1\*\lib\cmake\libssh2\*.cmake
# 中的 zlib/v1.3.1 路径替换为当前 zlib/v1.3.2 实际路径

# 重试配置
xmake f -a x64 -m release -p windows --target_type=server -y

# 编译
xmake -y
```

输出：`bin/<ModName>/<ModName>.dll` + `manifest.json` + `<ModName>.zip`

---

## 5. API 适配说明（levilamina 26.10.14）

模板/旧文档中的部分 API 在 26.10.14 已变更，必须按以下方式调用：

### 5.1 CommandRegistrar

```cpp
// ❌ 旧（26.10.14 中 getInstance() 需要 bool 参数，无 0 参重载）
auto& cmd = ll::command::CommandRegistrar::getInstance()
    .getOrCreateCommand("mycmd", "...", CommandPermissionLevel::Any);

// ✅ 新
auto& cmd = ll::command::CommandRegistrar::getServerInstance()
    .getOrCreateCommand("mycmd", "...", CommandPermissionLevel::Any);
```

### 5.2 EventBus::emplaceListener

```cpp
// ❌ 旧（单参数 lambda）
bus.emplaceListener<MyEvent>([](MyEvent& ev) { ... });

// ✅ 新（必须三参数：std::function + EventPriority + weak_ptr<Mod>）
#include <functional>
#include "ll/api/event/Listener.h"
#include "ll/api/mod/NativeMod.h"

ll::event::EventBus::getInstance().emplaceListener<MyEvent>(
    std::function<void(MyEvent&)>([](MyEvent& ev) { ... }),
    ll::event::EventPriority::Normal,
    ll::mod::NativeMod::current()
);
```

### 5.3 必需的 include

```cpp
#include "ll/api/command/CommandHandle.h"  // ★ CommandHandle 是前向声明，必须 include 才能调用 overload()
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"  // ★ emplaceListener 需要
#include "ll/api/event/command/ServerCommandRegisterEvent.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/mod/NativeMod.h"  // ★ NativeMod::current() 需要
```

---

## 6. 常见错误对照表

| 错误信息 | 根因 | 解决 |
|----------|------|------|
| `ninja: error: '.../zlib/v1.3.1/.../zlib.lib' missing` | libssh2 预编译包引用旧 zlib 路径 | 执行 patch 脚本（见 3.2） |
| `error: cannot assign to non-static data member 'length' with const-qualified type` | rapidjson 1.1.0 + clang-cl 不兼容 | `set_toolchains("msvc")` |
| `error C2660: 'CommandRegistrar::getInstance': function does not take 0 arguments` | API 变更 | 用 `getServerInstance()` |
| `error C2027: use of undefined type 'll::command::CommandHandle'` | 前向声明未展开 | `#include "ll/api/command/CommandHandle.h"` |
| `cannot convert argument 1 from '_Ty' to 'std::function<...>'` | emplaceListener 签名变更 | 用 `std::function` 显式包装 + 三参数 |
| `xmake` 安装 levilamina 卡 10 分钟 | GitHub 下载慢（国内） | 用 Watt Toolkit / ghproxy / 代理 |

---

## 7. 部署

构建产物结构：
```
bin/<ModName>/
├── <ModName>.dll      # 主文件
├── <ModName>.pdb      # 调试符号（可选）
└── manifest.json      # 模组清单
```

部署到 BDS 服务器：
```
<BDS>/plugins/<ModName>/
├── <ModName>.dll
└── manifest.json
```

启动 BDS，在游戏内输入 `/helloword` 测试。

---

## 8. 版本升级检查清单

升级 levilamina 版本时，检查：

- [ ] `xmake.lua` 中 `add_requires("levilamina x.x.x")` 版本号
- [ ] `tooth.json` 中依赖版本范围
- [ ] `CommandRegistrar` / `EventBus` 等 API 签名是否变化（查头文件）
- [ ] libssh2 / zlib 版本是否变化（若 zlib 升级，patch 脚本的正则也要改）
- [ ] 首次本地 `xmake f` 验证依赖能否解析
- [ ] CI 触发一次构建验证

---

## 9. 参考资源

- 模板仓库：https://github.com/LiteLDev/levilamina-mod-template
- xmake-repo（levilamina 包配方）：https://github.com/LiteLDev/xmake-repo
- LeviLamina 源码（查 API 头文件）：https://github.com/LiteLDev/LeviLamina
- xmake 官方仓库（libcurl/libssh2/zlib 包）：https://github.com/xmake-io/xmake-repo
