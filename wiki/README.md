# AnyLink Secure Client - 项目 Wiki

## 项目概述

AnyLink Secure Client 是一个跨平台的 SSL VPN 客户端，支持 OpenConnect 和 Cisco AnyConnect SSL VPN 协议。它使用 [sslcon](https://github.com/tlslink/sslcon) 作为核心与服务端通信。

### 支持的服务端

- [AnyLink](https://github.com/bjdgyc/anylink)
- [OpenConnect VPN server](https://gitlab.com/openconnect/ocserv)

### 支持的平台

- **Windows**: Windows 10 或更新版本
- **macOS**: 需要手动处理安全警告（未签名）
- **Linux**: Ubuntu 20.04 或更新版本

## 技术栈

| 技术 | 说明 |
|------|------|
| **开发语言** | C++14 |
| **GUI 框架** | Qt 6 (Core, GUI, Widgets, WebSockets) |
| **构建系统** | qmake (.pro 文件) |
| **VPN 核心** | sslcon (通过 JSON-RPC WebSocket 通信) |
| **密钥存储** | qtkeychain (系统密钥链) |
| **单实例控制** | SingleApplication |

## 文档目录

| 文档 | 说明 |
|------|------|
| [架构设计](./architecture.md) | 项目整体架构和设计模式 |
| [业务逻辑](./business-logic.md) | VPN 连接流程和核心业务逻辑 |
| [编码规范](./coding-standards.md) | 代码风格和命名约定 |
| [构建部署](./build-deploy.md) | 编译、打包和发布流程 |

### 模块文档

| 模块 | 说明 |
|------|------|
| [主窗口模块](./modules/anylink.md) | AnyLink 主窗口类 |
| [RPC 通信模块](./modules/jsonrpc.md) | JSON-RPC WebSocket 客户端 |
| [配置管理模块](./modules/config.md) | 应用配置管理 |
| [配置文件模块](./modules/profile.md) | VPN 连接配置文件管理 |
| [密钥链模块](./modules/keychain.md) | 系统密钥链集成 |
| [UI 组件模块](./modules/ui-components.md) | 自定义 UI 组件 |

## 项目结构

```
anylink-client/
├── src/                    # 源代码目录
│   ├── main.cpp           # 程序入口
│   ├── anylink.cpp/h      # 主窗口类
│   ├── anylink.ui         # 主窗口 UI 定义
│   ├── jsonrpcwebsocketclient.cpp/h  # RPC 客户端
│   ├── configmanager.cpp/h           # 配置管理
│   ├── profilemanager.cpp/h/ui       # 配置文件管理
│   ├── keychainclass.cpp/h           # 密钥链封装
│   ├── detaildialog.cpp/h/ui         # 详情对话框
│   ├── loading.cpp/h                 # 加载动画组件
│   ├── textbrowser.cpp/h/ui          # 文本浏览器
│   └── common.cpp/h                  # 公共定义
├── 3rdparty/              # 第三方库
│   ├── SingleApplication/ # 单实例控制
│   └── qtkeychain/        # 密钥链库
├── resource/              # 资源文件
│   ├── windows/           # Windows 资源
│   ├── mac/               # macOS 资源
│   ├── linux/             # Linux 资源
│   └── style.qss          # 样式表
├── i18n/                  # 国际化文件
├── installer/             # 安装程序配置
├── assets/                # 图片资源
└── anylink.pro            # qmake 项目文件
```

## 快速开始

### 环境要求

- Qt 6.x (Core, GUI, Widgets, WebSockets)
- C++14 兼容编译器
- qmake 或 Qt Creator

### 编译步骤

```bash
# 创建构建目录
mkdir build && cd build

# 生成 Makefile
qmake ../anylink.pro

# 编译
make
```

## 许可证

本项目基于开源许可证发布，详见 [LICENSE](../LICENSE) 文件。
