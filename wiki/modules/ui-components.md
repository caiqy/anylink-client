# UI 组件模块

## 概述

本模块包含项目中自定义的 UI 组件，包括加载动画、详情对话框和文本浏览器。

---

## Loading 加载动画

### 文件位置

- 头文件: `src/loading.h`
- 源文件: `src/loading.cpp`

### 类定义

```cpp
class Loading : public QWidget
{
    Q_OBJECT
public:
    explicit Loading(QWidget *parent = nullptr);
    void setDotCount(int);
    void setDotColor(const QColor &);
    void start();
    void stop();
    void setMaxDiameter(float);
    void setMinDiameter(float);
};
```

### 功能说明

圆形旋转点加载动画，用于显示连接进度。

### 默认配置

| 属性 | 默认值 |
|------|--------|
| 点数量 | 12 |
| 点颜色 | RGB(49, 177, 190) |
| 最大直径 | 5 |
| 最小直径 | 1 |
| 刷新间隔 | 90ms |

### 使用示例

```cpp
// UI 中使用 (promoted widget)
ui->progressBar->start();   // 开始动画
ui->progressBar->stop();    // 停止动画
```

### 实现原理

1. 计算圆形轨道上各点位置
2. 各点直径从大到小递减
3. 定时刷新，循环移动最大点位置
4. 使用 `QPainter` 绘制圆点

---

## DetailDialog 详情对话框

### 文件位置

- 头文件: `src/detaildialog.h`
- 源文件: `src/detaildialog.cpp`
- UI 文件: `src/detaildialog.ui`

### 类定义

```cpp
class DetailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DetailDialog(AnyLink *parent);
    ~DetailDialog();

    void setRoutes(const QJsonArray &excludes, const QJsonArray &includes);
    void clear();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
};
```

### 功能说明

显示 VPN 连接详细信息：
- 流量统计（发送/接收）
- 排除路由列表
- 包含路由列表

### 路由显示

```cpp
void DetailDialog::setRoutes(const QJsonArray &excludes, const QJsonArray &includes)
{
    // 解析 CIDR 格式
    for (int i = 0; i < excludes.size(); i++) {
        QPair<QHostAddress, int> cidr = QHostAddress::parseSubnet(excludes[i].toString());
        // 添加到表格
    }
}
```

### 流量统计

对话框显示时启动定时器，每秒更新流量统计：

```cpp
void DetailDialog::showEvent(QShowEvent *event)
{
    connect(&timer, &QTimer::timeout, this, [this]() {
        anylink->rpc->callAsync("stat", AnyLink::STAT, [this](const QJsonValue &result) {
            const QJsonObject &stat = result.toObject();
            ui->labelBytesSent->setText(format(stat["bytesSent"].toDouble()));
            ui->labelBytesReceived->setText(format(stat["bytesReceived"].toDouble()));
        });
    });
    timer.start(1000);
}
```

### 流量格式化

```cpp
QString DetailDialog::format(double bytes)
{
    // 使用 1000 进制（与 ip -h 命令一致）
    if (bytes < 1000) return QString("%1 B");
    else if (bytes < 1000000) return QString("%1 KB");
    else if (bytes < 1000000000) return QString("%1 MB");
    else return QString("%1 GB");
}
```

---

## TextBrowser 文本浏览器

### 文件位置

- 头文件: `src/textbrowser.h`
- 源文件: `src/textbrowser.cpp`
- UI 文件: `src/textbrowser.ui`

### 功能说明

用于显示：
- VPN 日志文件
- 安全提示文档（Markdown）

### 使用示例

```cpp
// 显示日志
TextBrowser textBrowser(tr("Log Viewer"), this);
textBrowser.setText(logContent);
textBrowser.exec();

// 显示 Markdown
TextBrowser textBrowser(tr("Security Tips"), this);
textBrowser.setMarkdown(markdownContent);
textBrowser.exec();
```

### 日志实时更新

使用 `QFileSystemWatcher` 监视日志文件变化：

```cpp
QFileSystemWatcher watcher;
watcher.addPath(filePath);

QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, [&]() {
    QFile updatedFile(filePath);
    if (updatedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        data = updatedFile.readAll();
        textBrowser.setText(data);
    }
});
```

---

## Common 公共定义

### 文件位置

- 头文件: `src/common.h`
- 源文件: `src/common.cpp`

### 全局变量

```cpp
extern QString agentName;        // "AnyLink Secure Client"
extern QString appVersion;       // "0.9.5"
extern QString configLocation;   // 配置文件目录
extern QString tempLocation;     // 临时文件目录
extern ConfigManager *configManager;  // 配置管理器实例
```

### 存储格式枚举

```cpp
enum SaveFormat {
    Json,    // JSON 格式
    Binary   // CBOR 二进制格式
};
```

### 消息对话框

```cpp
// 错误对话框
void error(const QString &message, QWidget *parent = nullptr);

// 提示对话框
void info(const QString &message, QWidget *parent = nullptr);
```

### 实现

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

void info(const QString &message, QWidget *parent)
{
    QMessageBox msgBox(QMessageBox::Information, 
                       QObject::tr("Tips"), 
                       message, 
                       QMessageBox::Ok, 
                       parent);
    msgBox.exec();
}
```

---

## macOS Dock 处理

### 文件位置

- 头文件: `src/macdockiconhandler.h`
- 源文件: `src/macdockiconhandler.mm` (Objective-C++)

### 功能说明

仅在 macOS 平台编译，处理 Dock 图标点击事件。

### 使用

```cpp
#if defined(Q_OS_MACOS)
MacDockIconHandler* dockIconHandler = MacDockIconHandler::instance();
connect(dockIconHandler, &MacDockIconHandler::dockIconClicked, this, [this]() { 
    showNormal(); 
});
trayIconMenu->setAsDockMenu();
#endif
```

---

## 样式表 (QSS)

### 文件位置

`resource/style.qss`

### 应用范围

仅在 Windows 和 Linux 平台加载：

```cpp
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    loadStyleSheet(":/resource/style.qss");
#endif
```

### 样式内容

```css
/* TabBar 样式 */
QTabBar::tab {
    height: 25px;
    color: rgb(79, 79, 79);
    background: transparent;
    font: 14px;
}

QTabBar::tab:selected {
    border-bottom: 2px solid rgb(45, 105, 65);
    color: black;
}

/* ListView 样式 */
QListView::item {
    margin: 2px;
    font: 18px;
}
```
