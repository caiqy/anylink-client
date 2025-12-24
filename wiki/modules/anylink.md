# 主窗口模块 (AnyLink)

## 概述

`AnyLink` 类是应用程序的主窗口，负责：
- 用户界面展示和交互
- VPN 连接状态管理
- 系统托盘集成
- 配置管理界面

## 文件位置

- 头文件: `src/anylink.h`
- 源文件: `src/anylink.cpp`
- UI 文件: `src/anylink.ui`

## 类定义

```cpp
class AnyLink : public QWidget
{
    Q_OBJECT

public:
    enum { STATUS, CONFIG, CONNECT, DISCONNECT, RECONNECT, INTERFACE, ABORT, STAT };

    AnyLink(QWidget *parent = nullptr);
    ~AnyLink();

    JsonRpcWebSocketClient *rpc = nullptr;

signals:
    void vpnConnected();
    void vpnClosed();
};
```

## RPC 消息 ID

| 枚举值 | 值 | 说明 |
|--------|-----|------|
| `STATUS` | 0 | 获取状态 |
| `CONFIG` | 1 | 配置下发 |
| `CONNECT` | 2 | 发起连接 |
| `DISCONNECT` | 3 | 断开连接 |
| `RECONNECT` | 4 | 快速重连 |
| `INTERFACE` | 5 | 网络接口 |
| `ABORT` | 6 | 异常断开 |
| `STAT` | 7 | 流量统计 |

## 主要成员

### UI 组件

| 成员 | 类型 | 说明 |
|------|------|------|
| `ui` | `Ui::AnyLink*` | UI 对象指针 |
| `trayIcon` | `QSystemTrayIcon*` | 系统托盘图标 |
| `trayIconMenu` | `QMenu*` | 托盘菜单 |
| `profileManager` | `ProfileManager*` | 配置文件管理器 |
| `detailDialog` | `DetailDialog*` | 详情对话框 |

### 状态图标

```cpp
QIcon iconConnected = QIcon(":/assets/connected.png");
QIcon iconNotConnected = QIcon(":/assets/notconnected.png");
QIcon iconConnecting = QIcon(":/assets/connecting.png");
```

### 状态变量

| 成员 | 类型 | 说明 |
|------|------|------|
| `m_vpnConnected` | `bool` | VPN 是否已连接 |
| `activeDisconnect` | `bool` | 是否主动断开 |
| `currentProfile` | `QJsonObject` | 当前使用的配置 |

## 核心方法

### 初始化方法

#### `afterShowOneTime()`
首次显示后执行的一次性初始化：

```cpp
void AnyLink::afterShowOneTime()
{
    createTrayActions();     // 创建托盘菜单动作
    createTrayIcon();        // 创建托盘图标
    initConfig();            // 初始化配置
    profileManager->afterShowOneTime();
    detailDialog = new DetailDialog(this);
    
    // 设置定时器和信号连接
    // 创建 RPC 客户端并连接
    rpc = new JsonRpcWebSocketClient(this);
    rpc->connectToServer(QUrl("ws://127.0.0.1:6210/rpc"));
}
```

### VPN 控制方法

#### `configVPN()`
下发配置到 vpnagent：

```cpp
void AnyLink::configVPN()
{
    QJsonObject args{
        {"log_level", debug ? "Debug" : "Info"},
        {"log_path", tempLocation},
        {"skip_verify", !block},
        {"cisco_compat", ciscoCompat},
        {"no_dtls", noDtls},
        {"agent_name", agentName},
        {"agent_version", appVersion}
    };
    rpc->callAsync("config", CONFIG, args, callback);
}
```

#### `connectVPN(bool reconnect)`
发起 VPN 连接：
- `reconnect = false`: 完整连接（需要认证）
- `reconnect = true`: 快速重连（复用 session）

#### `disconnectVPN()`
断开当前 VPN 连接。

#### `getVPNStatus()`
获取并显示 VPN 状态信息。

### UI 方法

#### `center()`
将窗口居中显示，考虑高 DPI 屏幕。

#### `loadStyleSheet()`
加载 QSS 样式表。

#### `resetVPNStatus()`
重置所有状态显示。

### 托盘相关

#### `createTrayActions()`
创建托盘菜单动作：
- 连接网关
- 断开网关
- 显示面板
- 退出

#### `createTrayIcon()`
创建系统托盘图标和菜单。

## 槽函数

### 自动连接槽

```cpp
void on_buttonConnect_clicked();  // 连接/断开按钮
void on_buttonProfile_clicked();  // 配置文件管理
void on_buttonViewLog_clicked();  // 查看日志
void on_buttonDetails_clicked();  // 显示详情
void on_buttonSecurityTips_clicked();  // 安全提示
```

## 信号

| 信号 | 说明 |
|------|------|
| `vpnConnected()` | VPN 连接成功时发出 |
| `vpnClosed()` | VPN 断开时发出 |

## 事件处理

### `closeEvent()`
窗口关闭事件：
- 已连接: 隐藏到托盘
- 未连接: 退出应用

### `showEvent()`
窗口显示事件，首次显示时触发初始化。

## UI 布局

主窗口包含以下区域：

1. **连接信息区**
   - 服务器选择下拉框
   - OTP 输入框
   - 连接/断开按钮

2. **状态显示区**
   - 通道类型 (TLS/DTLS)
   - 加密套件
   - 服务器/本地/VPN 地址
   - MTU/DNS

3. **设置选项卡**
   - 自动登录
   - 连接后最小化
   - 证书验证
   - 调试模式
   - 本地化
   - Cisco 兼容模式
   - DTLS 设置

4. **底部状态栏**
   - 连接状态文本
   - 加载动画

## 平台差异

### macOS
- 使用 `MacDockIconHandler` 处理 Dock 图标点击
- 托盘菜单设为 Dock 菜单
- 自定义窗口标志

### Windows
- 托盘图标点击显示主窗口
- 加载自定义样式表

### Linux
- 加载自定义样式表
- 设置窗口图标
