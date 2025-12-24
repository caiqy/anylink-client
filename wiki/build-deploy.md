# 构建与部署

## 开发环境配置

### 依赖项

| 依赖 | 版本 | 说明 |
|------|------|------|
| Qt | 6.x | Core, GUI, Widgets, WebSockets 模块 |
| C++ 编译器 | C++14 | MSVC/GCC/Clang |
| qmake | Qt 自带 | 构建系统 |

### 第三方库

项目通过 Git Submodule 引入第三方库：

```
3rdparty/
├── SingleApplication/  # 单实例控制
└── qtkeychain/         # 系统密钥链集成
```

初始化子模块：
```bash
git submodule update --init --recursive
```

## 本地构建

### Windows

1. **安装 Qt**
   - 下载 Qt Online Installer
   - 安装 Qt 6.x (MSVC 2019/2022 64-bit)
   - 确保安装 Qt WebSockets 模块

2. **使用 Qt Creator**
   ```
   打开 anylink.pro → Configure Project → Build
   ```

3. **命令行构建**
   ```batch
   mkdir build && cd build
   qmake ..\anylink.pro
   nmake  # 或 jom
   ```

### macOS

1. **安装依赖**
   ```bash
   brew install qt@6
   ```

2. **构建**
   ```bash
   mkdir build && cd build
   qmake ../anylink.pro
   make -j$(sysctl -n hw.ncpu)
   ```

3. **输出位置**
   ```
   out/bin/AnyLink.app
   ```

### Linux

1. **安装依赖 (Ubuntu/Debian)**
   ```bash
   sudo apt install qt6-base-dev qt6-websockets-dev \
                    libsecret-1-dev libgl1-mesa-dev
   ```

2. **构建**
   ```bash
   mkdir build && cd build
   qmake ../anylink.pro
   make -j$(nproc)
   ```

3. **输出位置**
   ```
   out/opt/anylink/bin/
   ```

## qmake 项目配置

### 核心配置 (anylink.pro)

```qmake
QT += core gui websockets widgets
CONFIG += c++14
VERSION = 0.9.5

# 第三方库
include(3rdparty/SingleApplication/singleapplication.pri)
include(3rdparty/qtkeychain/qtkeychain.pri)
DEFINES += QAPPLICATION_CLASS=QApplication
```

### 平台特定配置

#### Windows
```qmake
win32 {
    RC_ICONS = resource\windows\anylink.ico
    QMAKE_TARGET_PRODUCT = "AnyLink Secure Client"
    QMAKE_TARGET_COMPANY = "https://anylink.pro"
    DESTDIR = $$PWD/out/bin
}
```

#### macOS
```qmake
macx {
    QMAKE_TARGET_BUNDLE_PREFIX = pro.anylink
    TARGET = AnyLink
    ICON = resource/mac/anylink.icns
    QMAKE_INFO_PLIST = resource/mac/Info.plist
    DESTDIR = $$PWD/out/bin
}
```

#### Linux
```qmake
linux:!android {
    DESTDIR = $$PWD/out/opt/anylink/bin
}
```

## CI/CD 流程

### GitHub Actions

项目使用 GitHub Actions 自动化构建和发布，配置文件位于 `.github/workflows/release.yml`。

#### 触发条件
- 推送 tag（发布版本）
- 手动触发 workflow

#### 构建矩阵

| 平台 | 架构 | Qt 版本 |
|------|------|---------|
| Windows | x64 | 6.x |
| macOS | x64, arm64 | 6.x |
| Linux | amd64 | 6.x |

### 构建产物

| 平台 | 产物 | 说明 |
|------|------|------|
| Windows | `anylink-windows-amd64.exe` | NSIS 安装程序 |
| macOS (x64) | `anylink-macos-amd64.dmg` | DMG 镜像 |
| macOS (arm64) | `anylink-macos-arm64.dmg` | DMG 镜像 |
| Linux | `anylink-linux-amd64.tar.gz` | 压缩包 + .run 安装脚本 |

## 安装程序打包

### 目录结构

```
installer/
├── config/           # 安装程序配置
│   └── config.xml    # Qt IFW 配置
├── packages/         # 安装包定义
└── packer.sh         # 打包脚本
```

### Windows 安装程序

使用 Qt Installer Framework 或 NSIS：

```bash
# Qt IFW
binarycreator -c config/config.xml -p packages anylink-installer.exe
```

### macOS DMG

```bash
# 创建 DMG
hdiutil create -volname "AnyLink" -srcfolder out/bin/AnyLink.app \
               -ov -format UDZO anylink.dmg
```

### Linux 安装包

```bash
# 创建 tar.gz
cd out
tar -czvf anylink-linux-amd64.tar.gz opt/
```

## 部署注意事项

### Windows
- 需要 Visual C++ Redistributable
- 需要管理员权限安装 TAP 驱动

### macOS
- 未签名应用需要手动信任
- 下载后需移除隔离属性：
  ```bash
  xattr -r -d com.apple.quarantine anylink-*.dmg
  ```

### Linux
- 需要 root 权限创建 TUN 设备
- 可能需要安装 libsecret 依赖

## 第三方依赖详情

### Git Submodules

| 模块 | 路径 | 仓库 | 分支 |
|------|------|------|------|
| SingleApplication | `3rdparty/SingleApplication` | https://github.com/itay-grudev/SingleApplication.git | master |
| qtkeychain | `3rdparty/qtkeychain` | https://github.com/frankosterfeld/qtkeychain.git | main |

### VPN 核心组件 (sslcon)

项目使用 [sslcon](https://github.com/tlslink/sslcon) 作为 VPN 核心：

- **vpnagent**: VPN 代理服务进程
- **sslcon**: SSL VPN 连接核心

这些组件在 CI/CD 流程中自动下载并打包：

```bash
# Linux
wget https://github.com/caiqy/sslcon/releases/download/continuous/sslcon-linux-amd64.tar.gz
cp vpnagent sslcon out/opt/anylink/bin

# Windows
curl -L -O https://github.com/caiqy/sslcon/releases/download/continuous/sslcon-windows10-amd64.7z
cp vpnagent.exe sslcon.exe out/bin

# macOS
curl -L -O https://github.com/caiqy/sslcon/releases/download/continuous/sslcon-macOS-arm64.tar.gz
cp vpnagent sslcon AnyLink.app/Contents/MacOS
```

## 详细 CI/CD 配置

### 构建矩阵

| 构建目标 | 运行环境 | Qt 版本 | 架构 |
|----------|----------|---------|------|
| linux-latest | ubuntu-24.04 | 6.9.1 | linux_gcc_64 |
| linux | ubuntu-22.04 | 6.9.1 | linux_gcc_64 |
| linux-arm64 | ubuntu-24.04-arm | 6.9.1 | linux_gcc_arm64 |
| windows | windows-2022 | 6.10.0 | win64_msvc2022_64 |
| macos-arm64 | macos-14 | 6.9.1 | clang_64 |

### 打包工具

| 平台 | 工具 | 说明 |
|------|------|------|
| Linux | linuxdeployqt | 自动打包依赖库 |
| Windows | windeployqt | Qt 官方部署工具 |
| macOS | macdeployqt | Qt 官方部署工具 |
| 所有平台 | Qt IFW | Qt Installer Framework 创建安装程序 |

### Linux 特殊处理

- 使用 `linuxdeployqt` 打包所有依赖
- 集成 fcitx 输入法支持
- 创建 `.run` 自解压安装包

### 发布策略

- **continuous**: 分支推送触发，持续构建
- **tagged release**: 版本标签触发，正式发布

## 版本发布流程

1. **更新版本号**
   - `anylink.pro`: `VERSION = x.y.z`
   - `src/common.cpp`: `QString appVersion = "x.y.z";`

2. **更新 CHANGELOG**（如有）

3. **创建 Git Tag**
   ```bash
   git tag -a v0.9.5 -m "Release v0.9.5"
   git push origin v0.9.5
   ```

4. **GitHub Actions 自动构建**

5. **验证发布产物**

6. **发布 Release Notes**

## 调试配置

### 启用调试日志

在 UI 中勾选 "Debug" 选项，或通过配置：

```json
// config.json
{
    "debug": true
}
```

日志输出到临时目录：
```
Windows: %TEMP%/vpnagent.log
macOS: $TMPDIR/vpnagent.log
Linux: /tmp/vpnagent.log
```

### Qt Creator 调试

1. 打开 `anylink.pro`
2. 配置 Debug 构建
3. 设置断点
4. F5 启动调试

### 日志格式

```cpp
qSetMessagePattern("%{type}:[%{file}:%{line}]  %{message}");
```

输出示例：
```
debug:[anylink.cpp:123]  Connection established
```
