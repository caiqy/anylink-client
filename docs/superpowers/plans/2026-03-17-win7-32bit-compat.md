# Win7 32位兼容支持 Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 anylink-client 发布链中新增 Windows 7 32位（x86）构建目标，使用已有的 `sslcon-windows7-386.7z` 核心后端，并修复安装器中 VC++ 运行库的错误分支逻辑。

**Architecture:** 在 GitHub Actions 矩阵中新增 `windows-x86` 构建条目，使用 `win32_msvc2019` Qt 工具链编译 32位 GUI，打包时下载 `sslcon-windows7-386.7z` 替代 amd64 版本，安装器脚本补充 x86 VC++ redist 分支。

**Tech Stack:** GitHub Actions、Qt 5.15.2 (MSVC 2019 32-bit)、Qt Installer Framework 4.5.2、ilammy/msvc-dev-cmd、sslcon releases

---

## Chunk 1: CI 构建矩阵与归档名称

### Task 1: 新增 `windows-x86` 构建矩阵条目

**Files:**
- Modify: `.github/workflows/release.yml:15`（matrix.build 数组）
- Modify: `.github/workflows/release.yml:35-41`（matrix.include）

- [ ] **Step 1: 在 `matrix.build` 数组中追加 `windows-x86`**

  文件 `.github/workflows/release.yml` 第 15 行：

  ```yaml
  # 改前
  build: [linux-latest, linux, linux-arm64, windows, macos-arm64]
  
  # 改后
  build: [linux-latest, linux, linux-arm64, windows, windows-x86, macos-arm64]
  ```

- [ ] **Step 2: 在 `matrix.include` 中追加 `windows-x86` 条目**

  在 `windows` 条目（第 35-41 行）之后、`macos-arm64` 条目之前插入：

  ```yaml
          - build: windows-x86
            os: windows-2019
            qt: 5.15.2
            arch: win32_msvc2019
            qt-modules: ''
            qt-tools: 'tools_qtcreator'
            installer-name: anylink-windows-386.exe
  ```

- [ ] **Step 3: 验证 YAML 缩进与语法正确**

  在 runner 上 YAML 解析错误会直接导致 workflow 拒绝触发。使用以下命令检查语法（PyYAML 在 Ubuntu GitHub runner 上默认已安装；本地如未安装可用 `pip install pyyaml`）：

  ```bash
  python3 -c "import yaml; yaml.safe_load(open('.github/workflows/release.yml'))" && echo "YAML OK"
  ```

  预期输出：`YAML OK`（无报错即通过）

---

### Task 2: 更新 ARCHIVE_NAME 归档名称逻辑

**Files:**
- Modify: `.github/workflows/release.yml:49-64`（branches 归档名）
- Modify: `.github/workflows/release.yml:65-80`（tags 归档名）

- [ ] **Step 1: 在 branches 归档名逻辑中新增 `windows-x86` 分支**

  在 `elif [ "${{ matrix.build }}" = "windows" ]` 之后插入：

  ```bash
  elif [ "${{ matrix.build }}" = "windows-x86" ]; then
    echo "ARCHIVE_NAME=anylink-windows-386.zip" >> $GITHUB_ENV
  ```

- [ ] **Step 2: 在 tags 归档名逻辑中新增 `windows-x86` 分支**

  同样位置（tags 版本）插入：

  ```bash
  elif [ "${{ matrix.build }}" = "windows-x86" ]; then
    echo "ARCHIVE_NAME=anylink-${{ github.ref_name }}-windows-386.zip" >> $GITHUB_ENV
  ```

- [ ] **Step 3: Commit**

  ```bash
  git add .github/workflows/release.yml
  git commit -m "ci: add windows-x86 build matrix entry and archive name"
  ```

---

## Chunk 2: MSVC 环境与 Qt 构建步骤

### Task 3: 新增 x86 MSVC 开发环境 step

**Files:**
- Modify: `.github/workflows/release.yml:97-99`（Setup msvc step）

- [ ] **Step 1: 将现有 msvc-dev-cmd step 拆分为两个**

  现有（第 97-99 行）：
  ```yaml
      - if: matrix.build == 'windows'
        name: Setup msvc
        uses: ilammy/msvc-dev-cmd@v1
  ```

  改为：
  ```yaml
      - if: matrix.build == 'windows'
        name: Setup msvc
        uses: ilammy/msvc-dev-cmd@v1

      - if: matrix.build == 'windows-x86'
        name: Setup msvc (x86)
        uses: ilammy/msvc-dev-cmd@v1
        with:
          arch: x86
  ```

  > 说明：`ilammy/msvc-dev-cmd` 默认激活 x64 工具链；`arch: x86` 会切换到 32位 MSVC 环境，使 `cl.exe`、`link.exe` 和 `jom.exe` 均面向 x86 目标。

---

### Task 4: 扩展 Build 步骤以覆盖 `windows-x86`

**Files:**
- Modify: `.github/workflows/release.yml:129-132`（Build step Windows 分支）

- [ ] **Step 1: 将 Build 步骤中 Windows 条件改为显式 OR 判断**

  现有：
  ```bash
  elif [[ "${{ matrix.build }}" == "windows" ]]; then
  ```

  改为：
  ```bash
  elif [[ "${{ matrix.build }}" == "windows" || "${{ matrix.build }}" == "windows-x86" ]]; then
  ```

  > 说明：使用显式 OR 判断而非 `windows*` 通配，避免未来新增 `windows-arm64` 等目标时被意外纳入。`jom` 会读取当前 MSVC 工具链（已由上一步的 msvc-dev-cmd 正确设置为 x86 或 x64）。

- [ ] **Step 2: Commit**

  ```bash
  git add .github/workflows/release.yml
  git commit -m "ci: configure x86 msvc env and extend build step for windows-x86"
  ```

---

## Chunk 3: 打包步骤 — sslcon 与 VC++ Redist

### Task 5: 新增 `windows-x86` 的 Build archive 分支

**Files:**
- Modify: `.github/workflows/release.yml:176-188`（Build archive Windows 分支）

当前 `windows` 分支结束于第 188 行（`7z a -tzip ...`），在其之后、`elif [ "${{ matrix.build }}" = "macos-arm64" ]` 之前插入新分支。

- [ ] **Step 1: 在 Build archive 步骤中新增 `windows-x86` 分支**

  ```yaml
          elif [ "${{ matrix.build }}" = "windows-x86" ]; then
            windeployqt out/bin/anylink.exe --no-translations --no-system-d3d-compiler --no-opengl-sw --no-svg
            curl -k -L -O https://github.com/caiqy/sslcon/releases/download/latest/sslcon-windows7-386.7z
            7z x -y sslcon-*.7z
            cp vpnagent.exe sslcon.exe out/bin
            7z a installer/packages/root/data/anylink.7z ./out/bin/*
            cd installer
            curl -k -O -L https://mirrors.ustc.edu.cn/qtproject/archive/qt-installer-framework/4.3.0/QtInstallerFramework-windows-x86-4.3.0.exe
            ./QtInstallerFramework-windows-x86-4.3.0.exe --al --da -c -t `pwd`/ifw in
            curl -k -O -L https://aka.ms/vs/17/release/vc_redist.x86.exe
            cp vc_redist.x86.exe packages/root/data/
            sed '/<PersistentLocalCache>/d' config/config.xml > config/config-x86.xml
            ./ifw/bin/binarycreator --offline-only -c config/config-x86.xml -p packages ${{ matrix.installer-name }}
            editbin /subsystem:windows ${{ matrix.installer-name }}
            python -c "import pathlib, struct, sys; installer = pathlib.Path('${{ matrix.installer-name }}'); data = installer.read_bytes(); pe_offset = struct.unpack_from('<I', data, 0x3c)[0]; signature = data[pe_offset:pe_offset + 4]; machine = struct.unpack_from('<H', data, pe_offset + 4)[0]; print(f'{installer} PE signature={signature!r}, machine=0x{machine:04x}'); sys.exit(0 if signature == b'PE\0\0' and machine == 0x14c else 1)"
            7z a -tzip -r "${{ github.workspace }}"/archive/${{ env.ARCHIVE_NAME }} ${{ matrix.installer-name }}
  ```

  > 说明：
  > - IFW 4.5.2 不提供官方 x86 预编译包；`windows-x86` 使用最新可用的 IFW 4.3.0 x86 生成 32 位安装器
  > - IFW 4.3.0 x86 不支持 `PersistentLocalCache`，因此 x86 分支使用临时生成的 `config/config-x86.xml`
  > - PE 头校验要求安装器 `Machine` 为 `0x14c`，避免再次发布 64 位外层安装器
  > - `vc_redist.x86.exe` 从微软官方 aka.ms 短链下载后复制到安装包数据目录，供 `component.js` 引用
  > - `editbin /subsystem:windows` 修复安装器子系统标志，与 amd64 处理一致

- [ ] **Step 2: 验证 `windows-x86` tagged release 走正确的发布 step**

  查看文件末尾发布 step（第 206-227 行）：
  - continuous release（第 206-212 行）：条件 `startsWith(github.ref, 'refs/heads/')`，对所有构建生效，`windows-x86` 自动覆盖 ✅
  - tagged release（第 214-227 行）：
    - step 1（第 214 行）：条件 `matrix.build == 'windows'`，仅 amd64 ✅
    - step 2（第 222 行）：条件 `matrix.build != 'windows'`，`windows-x86` 会走此分支 ✅
  
  无需修改。

- [ ] **Step 3: Commit**

  ```bash
  git add .github/workflows/release.yml
  git commit -m "ci: add windows-x86 archive step with sslcon-386 and vc_redist.x86"
  ```

---

## Chunk 4: 安装器脚本修复

### Task 6: 修复 `component.js` VC++ 运行库 x86 分支

**Files:**
- Modify: `installer/packages/root/meta/component.js:68-72`

- [ ] **Step 1: 阅读当前逻辑，确认问题所在**

  当前（第 68-72 行）：
  ```js
  if (systemInfo.currentCpuArchitecture === "x86_64") {
      component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.x64.exe", "/norestart", "/q");
  } else {
      component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.arm64.exe", "/norestart", "/q"); 
  }
  ```

  问题：在 x86（32位）机器上，`currentCpuArchitecture` 返回 `"i386"`，会错误落入 `else` 分支安装 arm64 运行库。

- [ ] **Step 2: 插入 `i386` 分支**

  将上述代码替换为：

  ```js
  if (systemInfo.currentCpuArchitecture === "x86_64") {
      component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.x64.exe", "/norestart", "/q");
  } else if (systemInfo.currentCpuArchitecture === "i386") {
      component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.x86.exe", "/norestart", "/q");
  } else {
      component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.arm64.exe", "/norestart", "/q");
  }
  ```

  > 参考：Qt IFW `systemInfo.currentCpuArchitecture` 可能的值：`"x86_64"`, `"i386"`, `"arm64"` 等，与 Qt `QSysInfo::currentCpuArchitecture()` 返回值一致。

- [ ] **Step 3: 人工验证 `i386` 分支选择逻辑**

  `component.js` 在 QT IFW 运行时执行，无法直接单元测试，但可通过以下方式人工确认逻辑正确性：

  1. 在浏览器控制台或 Node.js 环境模拟：
     ```js
     const arch = "i386"; // 模拟 Win7 32位的 currentCpuArchitecture
     if (arch === "x86_64") {
         console.log("会安装: vc_redist.x64.exe");
     } else if (arch === "i386") {
         console.log("会安装: vc_redist.x86.exe");
     } else {
         console.log("会安装: vc_redist.arm64.exe");
     }
     // 预期输出：会安装: vc_redist.x86.exe
     ```
  2. 确认文件中出现了 `vc_redist.x86.exe` 的引用且拼写与 CI 打包步骤中 `cp vc_redist.x86.exe packages/root/data/` 一致。

- [ ] **Step 4: Commit**

  ```bash
  git add installer/packages/root/meta/component.js
  git commit -m "fix: add x86 vc_redist branch in installer component.js"
  ```

---

## Chunk 5: 文档更新

### Task 7: 更新 README.md Windows 系统要求

**Files:**
- Modify: `README.md:27`

- [ ] **Step 1: 修改 Windows 系统要求说明**

  改前：
  ```
  Please use Windows 7 64-bit or newer.
  ```

  改后：
  ```
  Please use Windows 7 (32-bit or 64-bit) or newer.
  ```

- [ ] **Step 2: Commit**

  ```bash
  git add README.md
  git commit -m "docs: update Windows system requirement to include 32-bit"
  ```

---

### Task 8: 更新 wiki/build-deploy.md

**Files:**
- Modify: `wiki/build-deploy.md`（多处）

- [ ] **Step 1: 更新本地构建 Windows 说明（第 34-35 行附近）**

  改前：
  ```markdown
  - 安装 Qt 5.15.2 (MSVC 2019 64-bit) 用于支持 Windows 7+
  - 或安装 Qt 6.x (MSVC 2019/2022 64-bit) 用于 Windows 10+
  ```

  改后：
  ```markdown
  - 安装 Qt 5.15.2 (MSVC 2019 64-bit) 用于支持 Windows 7+ 64-bit
  - 安装 Qt 5.15.2 (MSVC 2019 32-bit) 用于支持 Windows 7+ 32-bit
  - 或安装 Qt 6.x (MSVC 2019/2022 64-bit) 用于 Windows 10+
  ```

- [ ] **Step 2: 更新构建矩阵表（第 146-151 行）**

  改前：
  ```markdown
  | 平台 | 架构 | Qt 版本 | 系统要求 |
  |------|------|---------|----------|
  | Windows | x64 | 5.15.2 | Windows 7+ 64-bit |
  | macOS | x64, arm64 | 6.x | macOS 10.15+ |
  | Linux | amd64 | 6.x | Ubuntu 20.04+ |
  ```

  改后：
  ```markdown
  | 平台 | 架构 | Qt 版本 | 系统要求 |
  |------|------|---------|----------|
  | Windows | x64 | 5.15.2 | Windows 7+ 64-bit |
  | Windows | x86 | 5.15.2 | Windows 7+ 32-bit |
  | macOS | x64, arm64 | 6.x | macOS 10.15+ |
  | Linux | amd64 | 6.x | Ubuntu 20.04+ |
  ```

- [ ] **Step 3: 更新构建产物表（第 154-159 行）**

  将 Windows amd64 产物说明由 "NSIS 安装程序" 更正为 "IFW 安装程序"，并新增 x86 行：

  改前：
  ```markdown
  | Windows | `anylink-windows-amd64.exe` | NSIS 安装程序 |
  ```

  改后：
  ```markdown
  | Windows (x64) | `anylink-windows-amd64.exe` | IFW 安装程序（64位） |
  | Windows (x86) | `anylink-windows-386.exe` | IFW 安装程序（32位） |
  ```

- [ ] **Step 4: 更新详细 CI/CD 构建矩阵表（第 251-257 行）**

  在 `windows` 行之后新增：
  ```markdown
  | windows-x86 | windows-2019 | 5.15.2 | win32_msvc2019 |
  ```

- [ ] **Step 5: 更新 sslcon 下载示例（第 235-245 行）**

  将原有 Windows 示例替换为（注意加上解压步骤）：
  ```bash
  # Windows (64-bit / Win7+)
  curl -L -O https://github.com/caiqy/sslcon/releases/download/latest/sslcon-windows7-amd64.7z
  7z x -y sslcon-windows7-amd64.7z
  cp vpnagent.exe sslcon.exe out/bin
  
  # Windows (32-bit / Win7+)
  curl -L -O https://github.com/caiqy/sslcon/releases/download/latest/sslcon-windows7-386.7z
  7z x -y sslcon-windows7-386.7z
  cp vpnagent.exe sslcon.exe out/bin
  ```

- [ ] **Step 6: 更新子模块表（第 219-222 行）**

  在现有两行之后新增：
  ```markdown
  | sslcon | `3rdparty/sslcon` | https://github.com/caiqy/sslcon.git | win7-32bit |
  ```

- [ ] **Step 7: Commit**

  ```bash
  git add wiki/build-deploy.md
  git commit -m "docs: update wiki build-deploy for win7-32bit support"
  ```

---

## 验证清单

完成所有 tasks 后，push 到远端并手动触发 workflow，检查：

- [ ] `windows-x86` 构建任务出现在 Actions 列表并成功完成
- [ ] release assets 中出现 `anylink-windows-386.zip`
- [ ] 解压后检查三个核心二进制是否均为 32位 PE：

  **Linux/macOS（`file` 命令）：**
  ```bash
  file anylink.exe     # 预期：PE32 executable ... Intel 80386
  file vpnagent.exe    # 预期：PE32 executable ... Intel 80386
  file sslcon.exe      # 预期：PE32 executable ... Intel 80386
  ```

  **Windows（`dumpbin` 命令，三个文件分别执行）：**
  ```cmd
  dumpbin /headers anylink.exe   | findstr "machine"
  dumpbin /headers vpnagent.exe  | findstr "machine"
  dumpbin /headers sslcon.exe    | findstr "machine"
  ```
  三者均预期包含 `14C machine (x86)`

- [ ] 在 Win7 32位环境安装后，VC++ x86 运行库正常安装（无报错）
- [ ] VPN 连接功能在 Win7 32位下正常工作
