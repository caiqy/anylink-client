#include "reconnectdialog.h"
#include <QStyle>

ReconnectDialog::ReconnectDialog(const QString &errorMessage,
                                 bool quickReconnect,
                                 int countdownSeconds,
                                 QWidget *parent)
    : QDialog(parent)
    , m_remainingSeconds(countdownSeconds)
    , m_quickReconnect(quickReconnect)
{
    setupUI(errorMessage);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ReconnectDialog::onCountdownTick);
    m_timer->start(1000);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

ReconnectDialog::~ReconnectDialog()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void ReconnectDialog::setupUI(const QString &errorMessage)
{
    setWindowTitle(tr("Connection Error"));
    setMinimumWidth(350);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout *iconLayout = new QHBoxLayout();
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(style()->standardPixmap(QStyle::SP_MessageBoxWarning));
    iconLabel->setFixedSize(48, 48);
    iconLabel->setScaledContents(true);

    QLabel *titleLabel = new QLabel(tr("Connection interrupted"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    iconLayout->addWidget(iconLabel);
    iconLayout->addSpacing(10);
    iconLayout->addWidget(titleLabel);
    iconLayout->addStretch();
    mainLayout->addLayout(iconLayout);

    m_errorLabel = new QLabel(errorMessage, this);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet("color: #666666;");
    mainLayout->addWidget(m_errorLabel);

    m_countdownLabel = new QLabel(tr("Will retry in %1 seconds...").arg(m_remainingSeconds), this);
    m_countdownLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_countdownLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setMinimumWidth(100);
    connect(m_cancelButton, &QPushButton::clicked, this, &ReconnectDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    m_retryButton = new QPushButton(tr("Retry Now"), this);
    m_retryButton->setMinimumWidth(100);
    m_retryButton->setDefault(true);
    connect(m_retryButton, &QPushButton::clicked, this, &ReconnectDialog::onRetryClicked);
    buttonLayout->addWidget(m_retryButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void ReconnectDialog::onCountdownTick()
{
    m_remainingSeconds--;

    if (m_remainingSeconds <= 0) {
        m_timer->stop();
        emit retryRequested(m_quickReconnect);
        accept();
    } else {
        m_countdownLabel->setText(tr("Will retry in %1 seconds...").arg(m_remainingSeconds));
    }
}

void ReconnectDialog::onRetryClicked()
{
    m_timer->stop();
    emit retryRequested(m_quickReconnect);
    accept();
}

void ReconnectDialog::onCancelClicked()
{
    m_timer->stop();
    emit cancelRequested();
    reject();
}
