# 配置管理模块 (ConfigManager)

## 概述

`ConfigManager` 类负责管理应用程序的全局配置，包括配置的加载、保存和默认值管理。

## 文件位置

- 头文件: `src/configmanager.h`
- 源文件: `src/configmanager.cpp`

## 类定义

```cpp
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    explicit ConfigManager(QObject *parent = nullptr);

    QJsonObject config{
        {"lastProfile", ""},
        {"autoStart", false},
        {"autoLogin", false},
        {"minimize", true},
        {"block", true},
        {"debug", false},
        {"local", true},
        {"no_dtls", false},
        {"cisco_compat", false}
    };
    
    bool loadConfig(SaveFormat saveFormat);
    void saveConfig(SaveFormat saveFormat);
    void saveConfig();
};
```

## 配置项说明

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `lastProfile` | string | `""` | 上次使用的配置文件名称 |
| `autoStart` | bool | `false` | 开机时自动启动 AnyLink |
| `autoLogin` | bool | `false` | 启动时自动连接 VPN |
| `minimize` | bool | `true` | 连接成功后最小化到托盘 |
| `block` | bool | `true` | 启用服务器证书验证 |
| `debug` | bool | `false` | 启用调试日志输出 |
| `local` | bool | `true` | 启用本地化（中文界面） |
| `no_dtls` | bool | `false` | 禁用 DTLS 协议 |
| `cisco_compat` | bool | `false` | 启用 Cisco AnyConnect 兼容模式 |

## 配置文件位置

配置文件存储在系统标准配置目录：

```cpp
configLocation = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
```

| 平台 | 路径 |
|------|------|
| Windows | `C:/Users/<USER>/AppData/Local/AnyLink/` |
| macOS | `~/Library/Application Support/AnyLink/` |
| Linux | `~/.local/share/AnyLink/` |

## 存储格式

支持两种存储格式：

```cpp
enum SaveFormat {
    Json,    // JSON 格式 (config.json)
    Binary   // CBOR 格式 (config.dat)
};
```

默认使用 JSON 格式，便于调试和手动编辑。

## 核心方法

### `loadConfig()`

加载配置文件：

```cpp
bool ConfigManager::loadConfig(SaveFormat saveFormat)
{
    QFile loadFile(saveFormat == Json
                   ? configLocation + "/config.json"
                   : configLocation + "/config.dat");

    // 以 ReadWrite 模式打开，如果文件不存在会创建
    if (!loadFile.open(QIODevice::ReadWrite)) {
        error(tr("Couldn't open config file"));
        return false;
    }

    QByteArray data = loadFile.readAll();

    if (data.length()) {
        QJsonDocument loadDoc = (saveFormat == Json
                                 ? QJsonDocument::fromJson(data)
                                 : QJsonDocument(QCborValue::fromCbor(data)
                                                .toMap().toJsonObject()));
        
        const QJsonObject obj = loadDoc.object();
        // 合并已有配置（保留默认值）
        for (auto it = config.begin(); it != config.end(); it++) {
            if (obj.contains(it.key())) {
                config[it.key()] = obj.value(it.key());
            }
        }
    }
    return true;
}
```

**特点**：
- 文件不存在时自动创建
- 只覆盖文件中存在的配置项
- 保留默认值（向前兼容）

### `saveConfig(SaveFormat)`

保存配置到指定格式：

```cpp
void ConfigManager::saveConfig(SaveFormat saveFormat)
{
    QFile saveFile(saveFormat == Json
                   ? configLocation + "/config.json"
                   : configLocation + "/config.dat");

    if (!saveFile.open(QIODevice::WriteOnly)) {
        error(tr("Couldn't open config file"));
        return;
    }
    
    saveFile.write(saveFormat == Json
                   ? QJsonDocument(config).toJson()
                   : QCborValue::fromJsonValue(config).toCbor());
}
```

### `saveConfig()`

使用默认格式 (JSON) 保存：

```cpp
void ConfigManager::saveConfig()
{
    saveConfig(Json);
}
```

## 使用示例

### 初始化

```cpp
// main.cpp
configManager = new ConfigManager();
if (configManager->loadConfig(Json)) {
    // 配置加载成功
    if (configManager->config["local"].toBool()) {
        // 加载本地化翻译
    }
}
```

### 读取配置

```cpp
bool autoLogin = configManager->config["autoLogin"].toBool();
QString lastProfile = configManager->config["lastProfile"].toString();
```

### 修改配置

```cpp
configManager->config["autoLogin"] = true;
configManager->config["lastProfile"] = "MyVPN";
configManager->saveConfig();
```

### UI 绑定

```cpp
// 初始化 UI
ui->checkBoxAutoLogin->setChecked(configManager->config["autoLogin"].toBool());

// 绑定变更事件
connect(ui->checkBoxAutoLogin, &QCheckBox::toggled, this, [this](bool checked) {
    configManager->config["autoLogin"] = checked;
    saveConfig();
});
```

## 配置文件示例

```json
{
    "autoLogin": false,
    "block": true,
    "cisco_compat": false,
    "debug": false,
    "lastProfile": "Office VPN",
    "local": true,
    "minimize": true,
    "no_dtls": false
}
```

## 全局实例

`ConfigManager` 作为全局单例使用：

```cpp
// common.h
extern ConfigManager *configManager;

// common.cpp
ConfigManager *configManager = nullptr;

// main.cpp
configManager = new ConfigManager();
```

## 开机自启动功能

### AutoStartManager

`AutoStartManager` 类负责跨平台的开机自启动管理：

```cpp
// 头文件: src/autostartmanager.h
// 源文件: src/autostartmanager.cpp

class AutoStartManager {
public:
    static bool setAutoStart(bool enable);
    static bool isAutoStartEnabled();
};
```

### 平台实现

| 平台 | 实现方式 | 权限要求 |
|------|----------|----------|
| Windows | 注册表 `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run` | 用户级别 |
| macOS | `~/Library/LaunchAgents/` plist 文件 | 用户级别 |
| Linux | `~/.config/autostart/` desktop 文件 | 用户级别 |

### 使用示例

```cpp
// 设置开机自启动
AutoStartManager::setAutoStart(true);

// 检查状态
bool enabled = AutoStartManager::isAutoStartEnabled();
```

## 与其他模块的交互

### 配置下发到 vpnagent

```cpp
void AnyLink::configVPN()
{
    QJsonObject args{
        {"log_level", configManager->config["debug"].toBool() ? "Debug" : "Info"},
        {"skip_verify", !configManager->config["block"].toBool()},
        {"cisco_compat", configManager->config["cisco_compat"].toBool()},
        {"no_dtls", configManager->config["no_dtls"].toBool()},
        // ...
    };
    rpc->callAsync("config", CONFIG, args, callback);
}
```

### 自动登录触发

```cpp
if (configManager->config["autoLogin"].toBool()) {
    QString lastProfile = configManager->config["lastProfile"].toString();
    if (lastProfile == profile) {
        connectVPN();
    }
}
```
