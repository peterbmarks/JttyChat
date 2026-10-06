#pragma once

#include <QMainWindow>

class QLineEdit;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

// Top-level window: a scrolling column of speech-bubble messages above
// a text field + Send button, in the style of a simple iMessage-like client.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    // Appends a message bubble. isSent = true renders it as an outgoing
    // (right-aligned, blue) bubble; false renders it as incoming
    // (left-aligned, grey) bubble. This is the entry point a future
    // networking/backend layer would call for messages arriving from others.
    void addMessage(const QString &text, bool isSent);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void sendMessage();
    void simulateIncomingReply(const QString &originalText);

private:
    void updateBubbleWidths();
    void scrollToBottom();

    QScrollArea *m_scrollArea;
    QWidget *m_messagesContainer;
    QVBoxLayout *m_messagesLayout;
    QLineEdit *m_inputField;
    QPushButton *m_sendButton;
};
