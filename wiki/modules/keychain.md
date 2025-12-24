# 密钥链模块 (KeyChainClass)

## 概述

`KeyChainClass` 类封装了 `qtkeychain` 库，提供跨平台的系统密钥链访问，用于安全存储 VPN 密码。

## 文件位置

- 头文件: `src/keychainclass.h`
- 源文件: `src/keychainclass.cpp`

## 依赖

- 第三方库: `qtkeychain` (位于 `3rdparty/qtkeychain/`)

## 类定义

```cpp
class KeyChainClass: public QObject
{
    Q_OBJECT
public:
    KeyChainClass(QObject* parent = nullptr);

    Q_INVOKABLE void readKey(const QString& key);
    Q_INVOKABLE void writeKey(const QString& key, const QString& value);
    Q_INVOKABLE void deleteKey(const QString& key);

Q_SIGNALS:
    void keyStored(const QString& key);
    void keyRestored(const QString& key, const QString& value);
    void keyDeleted(const QString& key);
    void error(const QString& errorText);
};
```

## 平台实现

`qtkeychain` 在不同平台使用不同的后端：

| 平台 | 后端 |
|------|------|
| Windows | Windows Credential Manager |
| macOS | Keychain Services |
| Linux | Secret Service API (GNOME Keyring / KWallet) |

## 服务名称

所有密钥存储在同一服务下：

```cpp
QLatin1String("keychain.anylink")
```

## 核心方法

### `readKey()`

从密钥链读取密码：

```cpp
void KeyChainClass::readKey(const QString &key)
{
    ReadPasswordJob *readCredentialJob = new ReadPasswordJob(
        QLatin1String("keychain.anylink"));
    readCredentialJob->setKey(key);

    QObject::connect(readCredentialJob, &QKeychain::ReadPasswordJob::finished, 
        [=]() {
            if (readCredentialJob->error()) {
                emit error(tr("Read key failed: %1")
                    .arg(qPrintable(readCredentialJob->errorString())));
                return;
            }
            emit keyRestored(key, readCredentialJob->textData());
        });

    readCredentialJob->start();
}
```

### `writeKey()`

将密码写入密钥链：

```cpp
void KeyChainClass::writeKey(const QString &key, const QString &value)
{
    WritePasswordJob *writeCredentialJob = new WritePasswordJob(
        QLatin1String("keychain.anylink"));
    writeCredentialJob->setKey(key);

    QObject::connect(writeCredentialJob, &QKeychain::WritePasswordJob::finished, 
        [=]() {
            if (writeCredentialJob->error()) {
                emit error(tr("Write key failed: %1")
                    .arg(qPrintable(writeCredentialJob->errorString())));
                return;
            }
            emit keyStored(key);
        });

    writeCredentialJob->setTextData(value);
    writeCredentialJob->start();
}
```

### `deleteKey()`

从密钥链删除密码：

```cpp
void KeyChainClass::deleteKey(const QString &key)
{
    DeletePasswordJob *deleteCredentialJob = new DeletePasswordJob(
        QLatin1String("keychain.anylink"));
    deleteCredentialJob->setKey(key);

    QObject::connect(deleteCredentialJob, &QKeychain::DeletePasswordJob::finished, 
        [=]() {
            if (deleteCredentialJob->error()) {
                emit error(tr("Delete key failed: %1")
                    .arg(qPrintable(deleteCredentialJob->errorString())));
                return;
            }
            emit keyDeleted(key);
        });

    deleteCredentialJob->start();
}
```

## 信号

| 信号 | 参数 | 说明 |
|------|------|------|
| `keyStored` | `QString key` | 密码成功存储 |
| `keyRestored` | `QString key, QString value` | 密码成功读取 |
| `keyDeleted` | `QString key` | 密码成功删除 |
| `error` | `QString errorText` | 操作失败 |

## 异步操作

所有密钥链操作都是**异步**的：

```
调用 readKey()
       │
       ▼
 启动 Job.start()
       │
       ▼
   返回调用者
       │
       ▼
(后台执行密钥链操作)
       │
       ▼
 Job.finished 信号
       │
       ▼
发出 keyRestored 信号
```

## 使用示例

### 在 ProfileManager 中使用

```cpp
// 构造函数中连接信号
connect(&keyChain, &KeyChainClass::keyRestored, this,
        [this](const QString &key, const QString &password) {
    // 将密码合并到 profiles
    QJsonObject value = profiles.value(key).toObject();
    value.insert("password", password);
    profiles.insert(key, value);
    
    emit keyRestored(key);
});

// 加载时读取所有密钥
void ProfileManager::readKeys()
{
    for (auto it = profiles.begin(); it != profiles.end(); it++) {
        keyChain.readKey(it.key());
    }
}

// 保存时写入密钥
void ProfileManager::writeKeys()
{
    keyChain.writeKey(profileName, password);
}

// 删除配置时删除密钥
keyChain.deleteKey(profileName);
```

## 注意事项

### Job 自动删除

`qtkeychain` 的 Job 对象在 `finished` 信号发出后会自动删除：

```cpp
// Job will auto delete after emit finished signal
```

### 延迟处理

由于异步特性，密码恢复需要延迟处理：

```cpp
connect(profileManager, &ProfileManager::keyRestored, this, [this](const QString &profile) {
    // keychain 中的密码是异步获取的
    QTimer::singleShot(500, this, [this, profile]() {
        if (configManager->config["autoLogin"].toBool()) {
            // 自动登录
        }
    });
});
```

### 不能在退出后使用

```cpp
// 不能在调用 aboutToQuit 后使用
keyChain.writeKey(key, password);
```

## 错误处理

错误通过 `error` 信号报告：

```cpp
connect(&keyChain, &KeyChainClass::error, [](const QString& error) {
    qDebug() << "Keychain error:" << error;
});
```

常见错误：
- 密钥不存在
- 访问被拒绝
- 密钥链服务不可用（Linux）

## 安全性

- 密码以加密形式存储在系统密钥链中
- 不在内存中长期保存明文密码
- 配置文件中不包含密码字段
