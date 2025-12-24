#ifndef RECONNECTDIALOG_H
#define RECONNECTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ReconnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReconnectDialog(const QString &errorMessage,
                             bool quickReconnect = true,
                             int countdownSeconds = 5,
                             QWidget *parent = nullptr);
    ~ReconnectDialog();

signals:
    void retryRequested(bool quickReconnect);
    void cancelRequested();

private slots:
    void onCountdownTick();
    void onRetryClicked();
    void onCancelClicked();

private:
    QLabel *m_errorLabel;
    QLabel *m_countdownLabel;
    QPushButton *m_retryButton;
    QPushButton *m_cancelButton;
    QTimer *m_timer;
    int m_remainingSeconds;
    bool m_quickReconnect;

    void setupUI(const QString &errorMessage);
};

#endif // RECONNECTDIALOG_H
