# RPC 通信模块 (JsonRpcWebSocketClient)

## 概述

`JsonRpcWebSocketClient` 类封装了与 vpnagent 服务的 JSON-RPC 2.0 over WebSocket 通信。

## 文件位置

- 头文件: `src/jsonrpcwebsocketclient.h`
- 源文件: `src/jsonrpcwebsocketclient.cpp`

## 类定义

```cpp
class JsonRpcWebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit JsonRpcWebSocketClient(QObject *parent = nullptr);
    ~JsonRpcWebSocketClient();

    void connectToServer(const QUrl &url);
    bool isConnected();

    void callAsync(const QString &method, const int id, const QJsonObject &args,
                   std::function<void(QJsonValue)> callback = nullptr);
    void callAsync(const QString &method, const int id,
                   std::function<void(QJsonValue)> callback = nullptr);

    void registerCallback(const int id, std::function<void(QJsonValue)> callback);

signals:
    void error(const QString &error) const;
    void connected();
};
```

## 通信协议

### 服务端地址

```
ws://127.0.0.1:6210/rpc
```

### JSON-RPC 2.0 请求格式

```json
{
    "method": "connect",
    "jsonrpc": "2.0",
    "params": {
        "host": "vpn.example.com",
        "username": "user",
        "password": "pass"
    },
    "id": 2
}
```

### JSON-RPC 2.0 响应格式

**成功响应**:
```json
{
    "jsonrpc": "2.0",
    "result": "Connected successfully",
    "id": 2
}
```

**错误响应**:
```json
{
    "jsonrpc": "2.0",
    "error": {
        "code": -1,
        "message": "Authentication failed"
    },
    "id": 2
}
```

## 核心方法

### `connectToServer()`

建立 WebSocket 连接：

```cpp
void JsonRpcWebSocketClient::connectToServer(const QUrl &url)
{
    webSocket->open(url);
}
```

### `isConnected()`

检查连接状态：

```cpp
bool JsonRpcWebSocketClient::isConnected()
{
    return m_connected;
}
```

### `callAsync()` - 带参数

发送带参数的 RPC 请求：

```cpp
void JsonRpcWebSocketClient::callAsync(const QString &method, const int id, 
                                       const QJsonObject &args,
                                       std::function<void(QJsonValue)> callback)
{
    QJsonObject jsonRpc {
        {"method", method},
        {"jsonrpc", "2.0"},
        {"params", args},
        {"id", id},
    };

    QByteArray data = QJsonDocument(jsonRpc).toJson(QJsonDocument::Compact);

    if (webSocket->isValid()) {
        if (!m_callbacks.contains(id) && callback != nullptr) {
            m_callbacks.insert(id, callback);
        }
        webSocket->sendBinaryMessage(data);
    }
}
```

### `callAsync()` - 无参数

发送无参数的 RPC 请求：

```cpp
void JsonRpcWebSocketClient::callAsync(const QString &method, const int id, 
                                       std::function<void(QJsonValue)> callback)
{
    callAsync(method, id, QJsonObject(), callback);
}
```

### `registerCallback()`

注册被动回调（用于服务端推送消息）：

```cpp
void JsonRpcWebSocketClient::registerCallback(const int id, 
                                               std::function<void(QJsonValue)> callback)
{
    if (!m_callbacks.contains(id)) {
        m_callbacks.insert(id, callback);
    }
}
```

## 内部实现

### 消息接收处理

```cpp
void JsonRpcWebSocketClient::onTextMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        return;
    }
    
    const QJsonObject &result = doc.object();
    if (!result.contains("id")) {
        return;
    }

    int id = result.value("id").toInt();

    if (m_callbacks.contains(id)) {
        if (result.contains("error")) {
            m_callbacks.value(id)(result.value("error"));
        } else {
            m_callbacks.value(id)(result.value("result"));
        }
    }
}
```

### 连接状态管理

```cpp
// 构造函数中设置信号连接
connect(webSocket, &QWebSocket::connected, [this]() {
    m_connected = true;
    emit connected();
});

connect(webSocket, &QWebSocket::disconnected, [this]() {
    m_connected = false;
});

connect(webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), 
        [this]() {
    m_connected = false;
    emit error(webSocket->errorString());
});
```

## 信号

| 信号 | 说明 |
|------|------|
| `connected()` | WebSocket 连接成功时发出 |
| `error(QString)` | 连接错误时发出，携带错误信息 |

## 回调机制

### 主动回调
通过 `callAsync()` 注册，请求完成后自动调用一次。

### 被动回调
通过 `registerCallback()` 注册，等待服务端推送消息。

用于处理：
- `DISCONNECT`: 正常断开通知
- `ABORT`: 异常断开通知

## 使用示例

### 发起连接

```cpp
QJsonObject profile {
    {"host", "vpn.example.com"},
    {"username", "user"},
    {"password", "pass"}
};

rpc->callAsync("connect", CONNECT, profile, [this](const QJsonValue &result) {
    if (result.isObject()) {
        // 错误处理
        QString errorMsg = result.toObject().value("message").toString();
        error(errorMsg, this);
    } else {
        // 连接成功
        emit vpnConnected();
    }
});
```

### 注册断开回调

```cpp
rpc->registerCallback(DISCONNECT, [this](const QJsonValue &result) {
    ui->statusBar->setText(result.toString());
    emit vpnClosed();
});
```

### 获取状态

```cpp
rpc->callAsync("status", STATUS, [this](const QJsonValue &result) {
    const QJsonObject &status = result.toObject();
    if (!status.contains("code")) {
        // 更新 UI 显示
        ui->labelVPNAddress->setText(status["VPNAddress"].toString());
    }
});
```

## 生命周期管理

```cpp
// 构造
JsonRpcWebSocketClient::JsonRpcWebSocketClient(QObject *parent)
    : QObject(parent)
{
    webSocket = new QWebSocket();
    // 设置信号连接...
}

// 析构
JsonRpcWebSocketClient::~JsonRpcWebSocketClient()
{
    webSocket->close();
    webSocket->deleteLater();
}
```
