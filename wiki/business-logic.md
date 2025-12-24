# 业务逻辑

## 核心业务流程

### 1. 应用启动流程

```
┌──────────────────┐
│     main()       │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ 设置日志格式     │
│ 初始化配置路径   │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ 创建 SingleApp   │
│ (确保单实例)     │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ 创建 ConfigManager│
│ 加载应用配置     │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ 加载翻译文件     │
│ (如果启用本地化) │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ 创建 AnyLink     │
│ 主窗口并显示     │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ 进入事件循环     │
└──────────────────┘
```

### 2. 主窗口初始化 (`afterShowOneTime`)

主窗口首次显示后执行一次性初始化：

```cpp
void AnyLink::afterShowOneTime()
{
    // 1. 创建系统托盘菜单动作
    createTrayActions();
    
    // 2. 创建系统托盘图标
    createTrayIcon();
    
    // 3. 初始化配置界面
    initConfig();
    
    // 4. 初始化 ProfileManager
    profileManager->afterShowOneTime();
    
    // 5. 创建详情对话框
    detailDialog = new DetailDialog(this);
    
    // 6. 设置 DTLS 状态定时器
    connect(&timer, &QTimer::timeout, ...);
    
    // 7. 注册 VPN 状态变化处理
    connect(this, &AnyLink::vpnConnected, ...);
    connect(this, &AnyLink::vpnClosed, ...);
    
    // 8. 创建 RPC 客户端并连接
    rpc = new JsonRpcWebSocketClient(this);
    rpc->connectToServer(QUrl("ws://127.0.0.1:6210/rpc"));
}
```

### 3. VPN 连接流程

#### 3.1 配置下发 (`configVPN`)

连接建立后首先下发配置：

```cpp
void AnyLink::configVPN()
{
    QJsonObject args{
        {"log_level", debug ? "Debug" : "Info"},
        {"log_path", tempLocation},
        {"skip_verify", !block},        // 证书验证
        {"cisco_compat", ciscoCompat},  // Cisco 兼容模式
        {"no_dtls", noDtls},            // 禁用 DTLS
        {"agent_name", agentName},
        {"agent_version", appVersion}
    };
    rpc->callAsync("config", CONFIG, args, callback);
}
```

#### 3.2 发起连接 (`connectVPN`)

```cpp
void AnyLink::connectVPN(bool reconnect)
{
    // 1. 获取当前选择的配置文件
    QString name = ui->comboBoxHost->currentText();
    QJsonObject profile = profileManager->profiles[name].toObject();
    
    // 2. 处理 OTP（如果有）
    QString otp = ui->lineEditOTP->text();
    if (!otp.isEmpty()) {
        profile["password"] = profile["password"].toString() + otp;
    }
    
    // 3. 显示连接中状态
    ui->progressBar->start();
    trayIcon->setIcon(iconConnecting);
    
    // 4. 发送 RPC 请求
    QString method = reconnect ? "reconnect" : "connect";
    rpc->callAsync(method, id, profile, [this](const QJsonValue &result) {
        // 处理结果
    });
}
```

#### 3.3 断开连接 (`disconnectVPN`)

```cpp
void AnyLink::disconnectVPN()
{
    ui->progressBar->start();
    rpc->callAsync("disconnect", DISCONNECT);
    activeDisconnect = true;  // 标记主动断开
}
```

### 4. 自动重连机制

当 VPN 异常断开时，自动尝试重连：

```cpp
// 异常断开回调
rpc->registerCallback(ABORT, [this](const QJsonValue &result) {
    ui->statusBar->setText(result.toString());
    emit vpnClosed();
    
    if (!activeDisconnect) {
        // 1.5秒后快速重连（不需要重新认证）
        QTimer::singleShot(1500, this, [this]() { 
            connectVPN(true); 
        });
    }
});

// 快速重连失败时，进行完全重连
if (reconnect && result.isObject()) {
    QTimer::singleShot(3000, this, [this]() { 
        connectVPN();  // 完全重连
    });
}
```

### 5. 自动登录流程

```cpp
// ProfileManager 密钥恢复后触发
connect(profileManager, &ProfileManager::keyRestored, this, [this](const QString &profile) {
    QTimer::singleShot(500, this, [this, profile]() {
        if (configManager->config["autoLogin"].toBool()) {
            QString lastProfile = configManager->config["lastProfile"].toString();
            if (lastProfile == profile) {
                connectVPN();
            }
        }
    });
});
```

## RPC 通信协议

### JSON-RPC 2.0 格式

**请求格式**：
```json
{
    "method": "connect",
    "jsonrpc": "2.0",
    "params": {
        "host": "vpn.example.com",
        "username": "user",
        "password": "pass",
        "group": "",
        "secret": ""
    },
    "id": 2
}
```

**响应格式**：
```json
{
    "jsonrpc": "2.0",
    "result": "Connected successfully",
    "id": 2
}
```

### RPC 方法列表

| 方法 | ID | 说明 |
|------|-----|------|
| `status` | 0 | 获取 VPN 状态 |
| `config` | 1 | 下发配置 |
| `connect` | 2 | 发起连接 |
| `disconnect` | 3 | 断开连接 |
| `reconnect` | 4 | 快速重连 |
| `interface` | 5 | 获取网络接口信息 |
| `abort` | 6 | 异常断开通知（服务端推送） |
| `stat` | 7 | 获取流量统计 |

### 回调注册

```cpp
// 注册被动回调（服务端推送）
rpc->registerCallback(DISCONNECT, callback);  // 正常断开
rpc->registerCallback(ABORT, callback);       // 异常断开
```

## 配置管理

### 应用配置项

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `lastProfile` | string | "" | 上次使用的配置文件名 |
| `autoLogin` | bool | false | 启动时自动连接 |
| `minimize` | bool | true | 连接后最小化到托盘 |
| `block` | bool | true | 启用证书验证 |
| `debug` | bool | false | 调试日志 |
| `local` | bool | true | 启用本地化 |
| `no_dtls` | bool | false | 禁用 DTLS |
| `cisco_compat` | bool | false | Cisco 兼容模式 |

### 配置文件结构

```json
// profile.json (密码存储在系统密钥链)
{
    "ProfileName": {
        "host": "vpn.example.com",
        "username": "user",
        "group": "default",
        "secret": ""
    }
}
```

## 密钥链集成

### 密钥存储流程

```
保存配置文件
     │
     ▼
┌─────────────────┐
│ 提取密码字段    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 调用 writeKey   │
│ 存入系统密钥链  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 从 JSON 中移除  │
│ 密码字段        │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 保存到文件      │
└─────────────────┘
```

### 密钥读取流程

```
加载配置文件
     │
     ▼
┌─────────────────┐
│ 遍历所有配置    │
│ 调用 readKey    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 收到 keyRestored│
│ 信号            │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 合并密码到配置  │
│ profiles 对象   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 触发自动登录    │
│ (如果启用)      │
└─────────────────┘
```

## 动态域名分流

支持根据域名动态分流（Dynamic Split Tunneling）：

- **域名包含**：仅指定域名走 VPN
- **域名排除**：仅指定域名不走 VPN

由服务端配置，客户端自动应用分流规则。详见 [DynamicSplitTunneling.md](../DynamicSplitTunneling.md)。

## 窗口关闭行为

```cpp
void AnyLink::closeEvent(QCloseEvent *event)
{
    if (m_vpnConnected) {
        // VPN 连接中：隐藏到托盘
        hide();
        event->accept();
        trayIcon->show();
    } else {
        // 未连接：退出应用
        qApp->quit();
    }
}
```

## 退出清理

```cpp
connect(qApp, &QApplication::aboutToQuit, this, [this]() {
    if (m_vpnConnected) {
        disconnectVPN();  // 退出前断开 VPN
    }
});
```
