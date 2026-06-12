# 设计文档：Windows 7 32位兼容支持

**日期**：2026-03-17  
**状态**：待实现  
**分支**：win7-64bit

---

## 背景

sslcon 项目（`3rdparty/sslcon`，branch: `win7-32bit`）已完成对 Windows 7 32位的兼容改造，并在其 `latest` release 中发布了 `sslcon-windows7-386.7z`。

anylink-client 需要完成前端配套改造，使整条发布链支持 Win7 32位用户。

---

## 目标

在现有 Win7 64位（amd64）构建的基础上，**新增** Win7 32位（386）构建，做到：

1. CI 自动生成 `anylink-windows-386.exe` 安装程序和 `anylink-windows-386.zip` 压缩包
2. 32位安装程序正确安装 `vc_redist.x86.exe`，而非错误安装 arm64 版本
3. 文档（README、wiki）如实反映新的平台支持范围

---

## 变更范围

| 文件 | 改动类型 | 说明 |
|------|----------|------|
| `.github/workflows/release.yml` | 修改 | 新增 `windows-x86` 构建矩阵条目，更新归档名、sslcon 下载 URL、VC++ redist 打包 |
| `installer/packages/root/meta/component.js` | 修改 | 安装器修复 VC++ 运行库 x86 分支 |
| `README.md` | 修改 | 更新 Windows 系统要求说明 |
| `wiki/build-deploy.md` | 修改 | 更新构建矩阵表、构建产物表、sslcon 下载示例、子模块表 |

---

## 详细设计

### 1. `.github/workflows/release.yml`

#### 1.1 构建矩阵新增条目

在 `matrix.include` 末尾，`windows` 条目之后新增：

```yaml
- build: windows-x86
  os: windows-2019
  qt: 5.15.2
  arch: win32_msvc2019
  qt-modules: ''
  qt-tools: 'tools_qtcreator'
  installer-name: anylink-windows-386.exe
```

#### 1.2 归档名称（ARCHIVE_NAME）

在 branches 和 tags 两处 `if/elif` 链中各新增：

```bash
elif [ "${{ matrix.build }}" = "windows-x86" ]; then
  echo "ARCHIVE_NAME=anylink-windows-386.zip" >> $GITHUB_ENV
# (tags 版本)
elif [ "${{ matrix.build }}" = "windows-x86" ]; then
  echo "ARCHIVE_NAME=anylink-${{ github.ref_name }}-windows-386.zip" >> $GITHUB_ENV
```

#### 1.3 Build 步骤

Build 步骤现有逻辑：
```bash
elif [[ "${{ matrix.build }}" == "windows" ]]; then
  rm "C:\Program Files\Git\usr\bin\link.exe"
  qmake
  "${IQTA_TOOLS}/QtCreator/bin/jom/jom.exe" -j $(nproc) -f Makefile.Release
```

扩展条件以同时覆盖 `windows-x86`：
```bash
elif [[ "${{ matrix.build }}" == "windows" || "${{ matrix.build }}" == "windows-x86" ]]; then
```

#### 1.4 Build archive 步骤（Windows 核心打包）

现有 `windows` 分支下载 amd64 sslcon 并打包 x64 VC++ redist：
```bash
elif [ "${{ matrix.build }}" = "windows" ]; then
  windeployqt out/bin/anylink.exe ...
  curl -k -L -O .../sslcon-windows7-amd64.7z
  ...
  curl -k -O -L .../vc_redist.x64.exe
  ...
```

新增 `windows-x86` 分支，下载 386 sslcon 并打包 x86 VC++ redist：
```bash
elif [ "${{ matrix.build }}" = "windows-x86" ]; then
  windeployqt out/bin/anylink.exe ...
  curl -k -L -O https://github.com/caiqy/sslcon/releases/download/latest/sslcon-windows7-386.7z
  7z x -y sslcon-*.7z
  cp vpnagent.exe sslcon.exe out/bin
  7z a installer/packages/root/data/anylink.7z ./out/bin/*
  cd installer
  curl -k -O -L https://mirrors.ustc.edu.cn/qtproject/archive/qt-installer-framework/4.1.1/QtInstallerFramework-windows-x86-4.1.1.exe
  ./QtInstallerFramework-windows-x86-4.1.1.exe --al --da -c -t `pwd`/ifw in
  # 下载 x86 VC++ redist 并放入安装包目录
  curl -k -O -L https://aka.ms/vs/17/release/vc_redist.x86.exe
  cp vc_redist.x86.exe packages/root/data/
  ./ifw/bin/binarycreator --offline-only -c config/config.xml -p packages ${{ matrix.installer-name }}
  editbin /subsystem:windows ${{ matrix.installer-name }}
  python -c "import pathlib, struct, sys; installer = pathlib.Path('${{ matrix.installer-name }}'); data = installer.read_bytes(); pe_offset = struct.unpack_from('<I', data, 0x3c)[0]; signature = data[pe_offset:pe_offset + 4]; machine = struct.unpack_from('<H', data, pe_offset + 4)[0]; print(f'{installer} PE signature={signature!r}, machine=0x{machine:04x}'); sys.exit(0 if signature == b'PE\0\0' and machine == 0x14c else 1)"
  7z a -tzip -r "${{ github.workspace }}"/archive/${{ env.ARCHIVE_NAME }} ${{ matrix.installer-name }}
```

> **注意**：`windows-x86` 必须使用 x86 版 IFW 生成安装器。IFW 4.5.2 没有官方 x86 预编译包，因此使用 IFW 4.1.1 x86，并通过 PE 头校验确认安装器 `Machine` 为 `0x14c`。

#### 1.5 msvc-dev-cmd 环境

`ilammy/msvc-dev-cmd@v1` 默认激活 x64 工具链。对 `windows-x86` 构建需切换到 x86：

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

#### 1.6 发布条件

现有发布 step 的条件已是 `matrix.build == 'windows'`，无需改动——`windows-x86` 会通过 `continuous release`（`startsWith(github.ref, 'refs/heads/')`）和 tagged release 的通用分支自动发布：
- continuous release 的 `files: archive/${{ env.ARCHIVE_NAME }}` 对所有平台生效
- tagged release 现有逻辑：`windows` 有一个单独 step，其他平台用另一个 step。对 `windows-x86` 应归入"非 windows"的通用 tagged release step，确认当前条件 `matrix.build != 'windows'` 能覆盖即可。

---

### 2. `installer/packages/root/meta/component.js`

修复 Windows 下 VC++ 运行库安装逻辑，新增 `i386` 分支：

**改前：**
```js
if (systemInfo.currentCpuArchitecture === "x86_64") {
    component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.x64.exe", "/norestart", "/q");
} else {
    component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.arm64.exe", "/norestart", "/q"); 
}
```

**改后：**
```js
if (systemInfo.currentCpuArchitecture === "x86_64") {
    component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.x64.exe", "/norestart", "/q");
} else if (systemInfo.currentCpuArchitecture === "i386") {
    component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.x86.exe", "/norestart", "/q");
} else {
    component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.arm64.exe", "/norestart", "/q");
}
```

---

### 3. `README.md`

**改前：**
```
Please use Windows 7 64-bit or newer.
```

**改后：**
```
Please use Windows 7 (32-bit or 64-bit) or newer.
```

---

### 4. `wiki/build-deploy.md`

以下四处需更新：

#### 4.1 Windows 本地构建说明（第 34 行附近）

**改前：**
```
- 安装 Qt 5.15.2 (MSVC 2019 64-bit) 用于支持 Windows 7+
```

**改后：**
```
- 安装 Qt 5.15.2 (MSVC 2019 64-bit) 用于支持 Windows 7+ 64-bit
- 安装 Qt 5.15.2 (MSVC 2019 32-bit) 用于支持 Windows 7+ 32-bit
```

#### 4.2 构建矩阵表（第 146-151 行）

**改前：**
```markdown
| 平台 | 架构 | Qt 版本 | 系统要求 |
| Windows | x64 | 5.15.2 | Windows 7+ 64-bit |
```

**改后：**
```markdown
| 平台 | 架构 | Qt 版本 | 系统要求 |
| Windows | x64 | 5.15.2 | Windows 7+ 64-bit |
| Windows | x86 | 5.15.2 | Windows 7+ 32-bit |
```

#### 4.3 构建产物表（第 154-159 行）

新增 386 产物行：
```markdown
| Windows (x86) | `anylink-windows-386.exe` | IFW 安装程序（32位） |
```

#### 4.4 详细 CI/CD 构建矩阵表（第 251-257 行）

**改前：**
```markdown
| windows | windows-2019 | 5.15.2 | win64_msvc2019_64 |
```

**改后：**
```markdown
| windows     | windows-2019 | 5.15.2 | win64_msvc2019_64 |
| windows-x86 | windows-2019 | 5.15.2 | win32_msvc2019    |
```

#### 4.5 sslcon 下载示例（第 235-245 行）

更新 Windows 示例为正确的 release tag，并补充 386 说明：
```bash
# Windows (64-bit)
curl -L -O https://github.com/caiqy/sslcon/releases/download/latest/sslcon-windows7-amd64.7z

# Windows (32-bit)
curl -L -O https://github.com/caiqy/sslcon/releases/download/latest/sslcon-windows7-386.7z
```

#### 4.6 子模块表（第 219-222 行）

新增 sslcon 子模块行：
```markdown
| sslcon | `3rdparty/sslcon` | https://github.com/caiqy/sslcon.git | win7-32bit |
```

---

## 不在本次范围内

- Qt 工具链/源码层面的适配（已确认无需改动）
- Win7 32位实机/VM 集成测试（建议手动验证）
- 其他平台（Linux/macOS）无变化

---

## 验证清单

- [ ] CI `windows-x86` 构建任务成功完成
- [ ] 产物 `anylink-windows-386.zip` 出现在 release assets
- [ ] 解压后 `anylink.exe`、`vpnagent.exe`、`sslcon.exe` 均为 32位 PE
- [ ] 在 Win7 32位环境安装后，VC++ x86 运行库正常安装
- [ ] Win7 32位环境下 VPN 连接功能正常
