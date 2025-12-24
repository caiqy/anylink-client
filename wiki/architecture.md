# 架构设计

## 整体架构

AnyLink Secure Client 采用**客户端-代理**架构模式，GUI 客户端通过 JSON-RPC over WebSocket 与本地 VPN 代理服务 (`vpnagent`) 通信。

```
┌─────────────────────────────────────────────────────────────┐
│                     AnyLink Client (GUI)                     │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   AnyLink   │  │  Profile    │  │    DetailDialog     │  │
│  │  主窗口类   │  │  Manager    │  │    详情对话框       │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
│         │                │                     │             │
│         └────────────────┼─────────────────────┘             │
│                          │                                   │
│                ┌─────────▼─────────┐                        │
│                │ JsonRpcWebSocket  │                        │
│                │     Client        │                        │
│                └─────────┬─────────┘                        │
└──────────────────────────┼──────────────────────────────────┘
                           │ WebSocket (ws://127.0.0.1:6210/rpc)
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                    vpnagent (VPN 代理服务)                   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   sslcon    │  │   路由管理  │  │     TUN 设备        │  │
│  │  VPN 核心   │  │             │  │                     │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                           │
                           │ SSL/TLS, DTLS
                           ▼
                ┌─────────────────────┐
                │    VPN Server       │
                │ (AnyLink/ocserv)    │
                └─────────────────────┘
```

## 设计模式

### 1. 单实例模式 (Single Instance)

使用 `SingleApplication` 库确保只有一个客户端实例运行：

```cpp
// main.cpp
SingleApplication app(argc, argv);
QObject::connect(&app, &SingleApplication::instanceStarted, &w, &AnyLink::showNormal);
```

### 2. 观察者模式 (Observer Pattern)

使用 Qt 信号槽机制实现组件间解耦通信：

```cpp
// AnyLink 类中的信号
signals:
    void vpnConnected();
    void vpnClosed();

// 连接信号与槽
connect(this, &AnyLink::vpnConnected, this, [this]() {
    // 更新 UI 状态
});
```

### 3. 异步回调模式 (Async Callback)

RPC 调用采用异步回调处理响应：

```cpp
rpc->callAsync("connect", CONNECT, currentProfile, [this](const QJsonValue &result) {
    // 处理连接结果
});
```

### 4. 委托模式 (Delegate Pattern)

密钥链操作通过 `KeyChainClass` 委托给系统密钥链服务：

```cpp
class KeyChainClass: public QObject {
    Q_INVOKABLE void readKey(const QString& key);
    Q_INVOKABLE void writeKey(const QString& key, const QString& value);
    Q_INVOKABLE void deleteKey(const QString& key);
};
```

## 模块依赖关系

```
                    ┌──────────┐
                    │  main    │
                    └────┬─────┘
                         │
                         ▼
          ┌──────────────────────────────┐
          │          AnyLink             │
          │        (主窗口类)             │
          └──┬───────┬───────┬───────┬──┘
             │       │       │       │
    ┌────────▼──┐ ┌──▼────┐ ┌▼─────┐ ┌▼──────────────┐
    │ Profile   │ │Config │ │Detail│ │JsonRpcWebSocket│
    │ Manager   │ │Manager│ │Dialog│ │   Client       │
    └─────┬─────┘ └───────┘ └──────┘ └────────────────┘
          │
    ┌─────▼─────┐
    │ KeyChain  │
    │  Class    │
    └───────────┘
```

## 数据流

### VPN 连接流程

```
用户点击连接
     │
     ▼
┌─────────────────┐
│ 获取当前配置文件 │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 合并 OTP 到密码 │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│ 发送 RPC 请求   │────▶│   vpnagent      │
│   "connect"     │     │  处理连接请求    │
└────────┬────────┘     └────────┬────────┘
         │                       │
         ▼                       ▼
┌─────────────────┐     ┌─────────────────┐
│ 等待回调响应    │◀────│   返回结果      │
└────────┬────────┘     └─────────────────┘
         │
    ┌────┴────┐
    │         │
 成功▼      失败▼
┌────────┐ ┌────────┐
│发送信号│ │显示错误│
│vpnConnected│ │对话框  │
└────────┘ └────────┘
```

## 配置存储

### 配置文件位置

使用 Qt 标准路径存储配置：

```cpp
configLocation = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
// Windows: C:/Users/<USER>/AppData/Local/AnyLink
// macOS: ~/Library/Application Support/AnyLink
// Linux: ~/.local/share/AnyLink
```

### 配置文件格式

| 文件 | 格式 | 说明 |
|------|------|------|
| `config.json` | JSON | 应用配置 |
| `profile.json` | JSON | VPN 配置文件（不含密码） |

密码通过系统密钥链单独存储，不保存在配置文件中。

## VPN 核心组件

### sslcon 组件

项目依赖 [sslcon](https://github.com/tlslink/sslcon) 作为 VPN 核心，包含两个可执行文件：

| 组件 | 说明 |
|------|------|
| `vpnagent` | VPN 代理服务，提供 JSON-RPC WebSocket API |
| `sslcon` | SSL VPN 连接核心，处理实际的 VPN 隧道 |

### 组件通信

```
┌──────────────────┐
│  AnyLink Client  │
│     (GUI)        │
└────────┬─────────┘
         │ WebSocket JSON-RPC
         │ ws://127.0.0.1:6210/rpc
         ▼
┌──────────────────┐
│    vpnagent      │
│  (RPC 服务)      │
└────────┬─────────┘
         │ 进程调用
         ▼
┌──────────────────┐
│     sslcon       │
│  (VPN 隧道)      │
└────────┬─────────┘
         │ SSL/TLS, DTLS
         ▼
┌──────────────────┐
│   VPN Server     │
└──────────────────┘
```

### 安装位置

| 平台 | vpnagent 位置 |
|------|--------------|
| Windows | `C:\Program Files\AnyLink\bin\` |
| macOS | `AnyLink.app/Contents/MacOS/` |
| Linux | `/opt/anylink/bin/` |

## 安全设计

### 1. 密码存储

密码使用操作系统密钥链安全存储：
- **Windows**: Windows Credential Manager
- **macOS**: Keychain
- **Linux**: Secret Service API (GNOME Keyring / KWallet)

### 2. 证书验证

支持配置是否跳过服务器证书验证：

```cpp
QJsonObject args{
    {"skip_verify", !ui->checkBoxBlock->isChecked()},
    // ...
};
```

### 3. 本地通信

GUI 与 vpnagent 通过本地 WebSocket 通信 (`127.0.0.1:6210`)，不暴露到网络。

## 国际化支持

使用 Qt 翻译系统支持多语言：

```cpp
// 加载翻译文件
QTranslator myTranslator;
if(myTranslator.load(QLocale(), "anylink", "_", ":/i18n")) {
    app.installTranslator(&myTranslator);
}

// 在代码中使用 tr()
ui->buttonConnect->setText(tr("Connect"));
```

当前支持：
- 英文 (默认)
- 简体中文 (zh_CN)
