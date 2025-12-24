# 配置文件模块 (ProfileManager)

## 概述

`ProfileManager` 类负责管理 VPN 连接配置文件（Profile），包括创建、编辑、删除配置文件以及与系统密钥链的集成。

## 文件位置

- 头文件: `src/profilemanager.h`
- 源文件: `src/profilemanager.cpp`
- UI 文件: `src/profilemanager.ui`

## 类定义

```cpp
class ProfileManager : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileManager(QWidget *parent = nullptr);
    ~ProfileManager();

    QJsonObject profiles = {};
    QStringListModel *model;

    bool loadProfile(SaveFormat saveFormat);
    void saveProfile(SaveFormat saveFormat);
    void updateModel();
    void afterShowOneTime();

signals:
    void keyRestored(const QString &profile);

private:
    Ui::ProfileManager *ui;
    bool m_modified = false;
    KeyChainClass keyChain;

    void resetForm();
    void readKeys();
    void writeKeys();
};
```

## 配置文件结构

### 内存中的 Profile 对象

```cpp
QJsonObject profiles = {
    "ProfileName": {
        "host": "vpn.example.com",
        "username": "user",
        "password": "secret",      // 从密钥链加载
        "group": "default",
        "secret": "groupSecret"
    }
};
```

### 存储的 Profile 文件

密码不保存在文件中：

```json
{
    "Office VPN": {
        "host": "vpn.company.com",
        "username": "john",
        "group": "employees",
        "secret": ""
    },
    "Home VPN": {
        "host": "home.example.com",
        "username": "admin",
        "group": "",
        "secret": ""
    }
}
```

## 存储位置

```
Windows: C:/Users/<USER>/AppData/Local/AnyLink/profile.json
macOS:   ~/Library/Application Support/AnyLink/profile.json
Linux:   ~/.local/share/AnyLink/profile.json
```

## 核心方法

### `loadProfile()`

加载配置文件并读取密钥：

```cpp
bool ProfileManager::loadProfile(SaveFormat saveFormat)
{
    QFile loadFile(saveFormat == Json
                   ? configLocation + "/profile.json"
                   : configLocation + "/profile.dat");

    if (!loadFile.open(QIODevice::ReadWrite)) {
        error(tr("Couldn't open profile file"), parentWidget());
        return false;
    }

    QByteArray data = loadFile.readAll();

    if (data.length()) {
        QJsonDocument loadDoc = (saveFormat == Json
                                 ? QJsonDocument::fromJson(data)
                                 : QJsonDocument(QCborValue::fromCbor(data)
                                                .toMap().toJsonObject()));
        profiles = loadDoc.object();
        readKeys();  // 从密钥链读取密码
    }
    return true;
}
```

### `saveProfile()`

保存配置文件（移除密码后）：

```cpp
void ProfileManager::saveProfile(SaveFormat saveFormat)
{
    if (m_modified) {
        QFile saveFile(...);
        
        writeKeys();  // 先保存密码到密钥链
        
        // 创建不含密码的副本
        QJsonObject saveProfiles = profiles;
        for (auto it = saveProfiles.begin(); it != saveProfiles.end(); it++) {
            QJsonObject value = it.value().toObject();
            value.remove("password");
            saveProfiles.insert(it.key(), value);
        }
        
        saveFile.write(QJsonDocument(saveProfiles).toJson());
    }
}
```

### `updateModel()`

更新 UI 列表模型：

```cpp
void ProfileManager::updateModel()
{
    model->setStringList(profiles.keys());
}
```

### `afterShowOneTime()`

设置 UI 事件绑定：

```cpp
void ProfileManager::afterShowOneTime()
{
    // 列表选择变化
    connect(ui->listProfile->selectionModel(),
            &QItemSelectionModel::currentRowChanged, ...);
    
    // 保存按钮
    connect(ui->buttonSave, &QPushButton::clicked, ...);
    
    // 新建按钮
    connect(ui->buttonNew, &QPushButton::clicked, ...);
    
    // 删除按钮
    connect(ui->buttonDelete, &QPushButton::clicked, ...);
}
```

### `resetForm()`

重置表单为新建状态：

```cpp
void ProfileManager::resetForm()
{
    ui->listProfile->selectionModel()->clear();
    
    ui->buttonNew->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
    
    ui->lineEditName->setEnabled(true);
    ui->lineEditName->setClearButtonEnabled(true);
    
    // 清空所有输入框
    ui->lineEditName->clear();
    ui->lineEditHost->clear();
    ui->lineEditUsername->clear();
    ui->lineEditPassword->clear();
    ui->lineEditGroup->clear();
    ui->lineEditSecretkey->clear();
}
```

## 密钥链集成

### 读取密码

```cpp
void ProfileManager::readKeys()
{
    for (auto it = profiles.begin(); it != profiles.end(); it++) {
        keyChain.readKey(it.key());
    }
}
```

### 保存密码

```cpp
void ProfileManager::writeKeys()
{
    const QString key = ui->lineEditName->text();
    const QString password = ui->lineEditPassword->text();
    keyChain.writeKey(key, password);
}
```

### 密钥恢复信号

```cpp
// 构造函数中
connect(&keyChain, &KeyChainClass::keyRestored, this,
        [this](const QString &key, const QString &password) {
    QJsonObject value = profiles.value(key).toObject();
    value.insert("password", password);
    profiles.insert(key, value);
    
    emit keyRestored(key);  // 通知主窗口
});
```

## UI 交互流程

### 选择配置文件

```
点击列表项
    │
    ▼
触发 currentRowChanged
    │
    ▼
加载配置到表单
    │
    ├─ 名称 (只读)
    ├─ 主机
    ├─ 用户名
    ├─ 密码
    ├─ 组
    └─ 密钥
```

### 新建配置文件

```
点击 "新建"
    │
    ▼
resetForm()
    │
    ▼
用户填写表单
    │
    ▼
点击 "保存"
    │
    ▼
验证必填项
    │
    ▼
添加到 profiles
    │
    ▼
saveProfile()
```

### 删除配置文件

```
选择配置文件
    │
    ▼
点击 "删除"
    │
    ▼
从 profiles 移除
    │
    ▼
从密钥链删除
    │
    ▼
saveProfile()
```

## 信号

| 信号 | 参数 | 说明 |
|------|------|------|
| `keyRestored` | `QString profile` | 密码从密钥链恢复后发出 |

## 与主窗口的交互

### 初始化

```cpp
// AnyLink 构造函数
profileManager = new ProfileManager(this);

if (profileManager->loadProfile(Json)) {
    profileManager->updateModel();
    ui->comboBoxHost->setModel(profileManager->model);
    
    // 恢复上次选择
    QString lastProfile = configManager->config["lastProfile"].toString();
    if (!lastProfile.isEmpty()) {
        ui->comboBoxHost->setCurrentText(lastProfile);
    }
}
```

### 自动登录触发

```cpp
connect(profileManager, &ProfileManager::keyRestored, this, [this](const QString &profile) {
    QTimer::singleShot(500, this, [this, profile]() {
        if (configManager->config["autoLogin"].toBool()) {
            if (configManager->config["lastProfile"].toString() == profile) {
                connectVPN();
            }
        }
    });
});
```

### 获取当前配置

```cpp
QString name = ui->comboBoxHost->currentText();
QJsonObject profile = profileManager->profiles[name].toObject();
```

## 安全提示

首次保存配置时显示安全提示：

```cpp
if (profiles.isEmpty()) {
    info(tr("This software can save passwords in the Keychain of the "
            "operating system to avoid plaintext passwords, but you should "
            "evaluate whether your usage scenarios allow saving passwords "
            "and avoid potential security risks."), this);
}
```
