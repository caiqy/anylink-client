# 编码规范

## 代码风格

### 命名约定

#### 类名
- 使用 **PascalCase**
- 示例: `AnyLink`, `ProfileManager`, `JsonRpcWebSocketClient`

#### 成员变量
- 私有成员使用 `m_` 前缀或无前缀
- 示例: `m_vpnConnected`, `m_modified`
- UI 指针: `ui`
- 其他指针无前缀: `rpc`, `trayIcon`

#### 局部变量和参数
- 使用 **camelCase**
- 示例: `saveFormat`, `currentProfile`, `reconnect`

#### 常量和枚举
- 枚举值使用 **UPPER_CASE**
- 示例: `STATUS`, `CONFIG`, `CONNECT`, `DISCONNECT`

#### 信号和槽
- 信号: 使用动词过去式或名词
  - `vpnConnected()`, `vpnClosed()`, `keyRestored()`
- 槽函数: 使用 `on_` 前缀（自动连接）或描述性名称
  - `on_buttonConnect_clicked()`, `connectVPN()`, `disconnectVPN()`

### 文件命名

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| 头文件 | 全小写，与类名对应 | `anylink.h`, `configmanager.h` |
| 源文件 | 全小写，与类名对应 | `anylink.cpp`, `configmanager.cpp` |
| UI 文件 | 全小写，与类名对应 | `anylink.ui`, `profilemanager.ui` |

### 代码格式

#### 大括号风格
```cpp
// 函数定义
void AnyLink::connectVPN(bool reconnect)
{
    // 代码
}

// 条件语句
if (condition) {
    // 代码
} else {
    // 代码
}

// 类定义
class AnyLink : public QWidget
{
    Q_OBJECT
public:
    // ...
};
```

#### 缩进
- 使用 **4 个空格**（非 Tab）
- 连续参数换行时对齐

```cpp
rpc->callAsync("config", CONFIG, args, [this](const QJsonValue &result) {
    ui->statusBar->setText(result.toString());
});
```

#### 空行
- 函数之间空一行
- 逻辑块之间空一行
- 类成员分组之间空一行

### 头文件规范

#### Include Guard
使用传统的 `#ifndef` 守卫：

```cpp
#ifndef ANYLINK_H
#define ANYLINK_H

// 内容

#endif // ANYLINK_H
```

#### Include 顺序
1. 对应的头文件（.cpp 文件）
2. Qt 头文件
3. 第三方库头文件
4. 项目内头文件

```cpp
#include "anylink.h"          // 对应头文件
#include <QCloseEvent>        // Qt 头文件
#include <QFile>
#include <QtWidgets>
#include "configmanager.h"    // 项目头文件
#include "profilemanager.h"
```

#### 前向声明
优先使用前向声明减少编译依赖：

```cpp
// anylink.h
class JsonRpcWebSocketClient;  // 前向声明
class ProfileManager;
class DetailDialog;
class QSystemTrayIcon;
class QMenu;
```

## Qt 特定规范

### 信号槽连接

#### 优先使用新式语法
```cpp
// 推荐
connect(ui->checkBoxAutoLogin, &QCheckBox::toggled, this, [this](bool checked) {
    configManager->config["autoLogin"] = checked;
    saveConfig();
});

// 不推荐（旧式）
connect(sender, SIGNAL(signal()), receiver, SLOT(slot()));
```

#### 自动连接命名
使用 `on_<objectName>_<signalName>` 格式：

```cpp
private slots:
    void on_buttonConnect_clicked();
    void on_buttonProfile_clicked();
```

### 内存管理

#### 父子对象关系
利用 Qt 对象树自动管理内存：

```cpp
// 正确：指定父对象
trayIcon = new QSystemTrayIcon(this);
rpc = new JsonRpcWebSocketClient(this);

// 手动删除（如果没有父对象）
AnyLink::~AnyLink() { delete ui; }
```

### 资源文件

#### QRC 文件
```xml
<RCC>
    <qresource prefix="/">
        <file>resource/style.qss</file>
        <file>assets/connected.png</file>
    </qresource>
</RCC>
```

#### 资源路径
```cpp
QIcon iconConnected = QIcon(":/assets/connected.png");
loadStyleSheet(":/resource/style.qss");
```

### 国际化

#### 可翻译字符串
使用 `tr()` 函数包裹用户可见字符串：

```cpp
ui->buttonConnect->setText(tr("Connect"));
error(tr("Couldn't open config file"));
```

#### 翻译文件
```cpp
if (myTranslator.load(QLocale(), "anylink", "_", ":/i18n")) {
    app.installTranslator(&myTranslator);
}
```

## 错误处理

### 错误对话框
```cpp
void error(const QString &message, QWidget *parent)
{
    QMessageBox msgBox(QMessageBox::Critical, 
                       QObject::tr("Error"), 
                       message, 
                       QMessageBox::Ok, 
                       parent);
    msgBox.exec();
}
```

### 文件操作
```cpp
QFile loadFile(filePath);
if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    error(tr("Couldn't open log file"), this);
    return;
}
```

### RPC 错误处理
```cpp
rpc->callAsync("connect", CONNECT, profile, [this](const QJsonValue &result) {
    if (result.isObject()) {  // 错误对象
        error(result.toObject().value("message").toString(), this);
    } else {
        // 成功处理
    }
});
```

## 注释规范

### 文件头注释
不强制要求文件头注释，但复杂文件可添加说明。

### 函数注释
使用 Doxygen 风格（可选）：

```cpp
/**
 * @brief don't send anything, just wait for the server to call.
 * @param id
 * @param callback
 */
void JsonRpcWebSocketClient::registerCallback(const int id, 
                                               std::function<void(QJsonValue)> callback)
```

### 行内注释
```cpp
// 每隔 60 秒获取 DTLS 状态
connect(&timer, &QTimer::timeout, this, [this]() {
    // ...
});

// 快速重连，不需要再次进行用户认证
QTimer::singleShot(1500, this, [this]() { connectVPN(true); });
```

### 中文注释
项目中允许使用中文注释，保持简洁：

```cpp
// 需要联合使用 QSysInfo::kernelType() 和 QSysInfo::productType()
// 将窗口移动到居中位置
// 不能在调用 aboutToQuit 后使用
```

## 平台兼容性

### 条件编译
```cpp
#if defined(Q_OS_MACOS)
    #include "macdockiconhandler.h"
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    loadStyleSheet(":/resource/style.qss");
#endif
```

### 平台特定代码
```cpp
#ifndef Q_OS_MACOS
    layout()->removeItem(ui->topSpacer);
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
#else
    setWindowFlags(Qt::CustomizeWindowHint | ...);
#endif
```

## 版本控制

### 版本号定义
在 `.pro` 文件和 `common.cpp` 中同步更新：

```cpp
// anylink.pro
VERSION = 0.9.5

// common.cpp
QString appVersion = "0.9.5";
```

### Git 忽略
参见 `.gitignore` 文件，主要忽略：
- 构建输出 (`out/`, `build/`)
- 用户配置 (`*.user`)
- 打包文件 (`*.7z`, `*.tar.gz`, `*.dmg`)
