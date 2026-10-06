#include "MainWindow.h"
#include "ChatBubble.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace {
constexpr int kMaxBubbleWidthFraction = 70; // percent of viewport width
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("JttyChat"));
    resize(420, 640);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Scrolling message history.
    m_scrollArea = new QScrollArea(central);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(QStringLiteral("background-color: #ffffff;"));

    m_messagesContainer = new QWidget(m_scrollArea);
    m_messagesContainer->setStyleSheet(QStringLiteral("background-color: #ffffff;"));
    m_messagesLayout = new QVBoxLayout(m_messagesContainer);
    m_messagesLayout->setContentsMargins(4, 8, 4, 8);
    m_messagesLayout->setSpacing(4);
    m_messagesLayout->addStretch(1);

    m_scrollArea->setWidget(m_messagesContainer);
    rootLayout->addWidget(m_scrollArea, 1);

    // Bottom input bar.
    auto *inputBar = new QWidget(central);
    inputBar->setStyleSheet(QStringLiteral("background-color: #f2f2f2;"));
    auto *inputLayout = new QHBoxLayout(inputBar);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(8);

    m_inputField = new QLineEdit(inputBar);
    m_inputField->setPlaceholderText(QStringLiteral("Text Message"));
    m_inputField->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  border: 1px solid #c8c8c8;"
        "  border-radius: 16px;"
        "  padding: 8px 14px;"
        "  font-size: 14px;"
        "  background-color: #ffffff;"
        "}"));

    m_sendButton = new QPushButton(QStringLiteral("Send"), inputBar);
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_sendButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #0b93f6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 16px;"
        "  padding: 8px 18px;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #a8d4fb;"
        "}"
        "QPushButton:hover:!disabled {"
        "  background-color: #0a84dd;"
        "}"));

    inputLayout->addWidget(m_inputField, 1);
    inputLayout->addWidget(m_sendButton, 0);

    rootLayout->addWidget(inputBar, 0);

    setCentralWidget(central);

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(m_inputField, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);

    m_inputField->setFocus();
}

void MainWindow::sendMessage()
{
    const QString text = m_inputField->text().trimmed();
    if (text.isEmpty())
        return;

    addMessage(text, true);
    m_inputField->clear();

    // Stand-in for a real backend: echo the message back shortly after,
    // rendered as an incoming bubble, so received-message styling is visible
    // without needing a network layer wired up yet.
    QTimer::singleShot(600, this, [this, text]() { simulateIncomingReply(text); });
}

void MainWindow::simulateIncomingReply(const QString &originalText)
{
    addMessage(QStringLiteral("You said: \"%1\"").arg(originalText), false);
}

void MainWindow::addMessage(const QString &text, bool isSent)
{
    auto *bubble = new ChatBubble(text, isSent, m_messagesContainer);

    // Insert before the trailing stretch so new bubbles land at the bottom
    // of the scroll area while older ones stay pinned to the top.
    const int insertIndex = m_messagesLayout->count() - 1;
    m_messagesLayout->insertWidget(insertIndex, bubble);

    updateBubbleWidths();
    scrollToBottom();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateBubbleWidths();
}

void MainWindow::updateBubbleWidths()
{
    const int viewportWidth = m_scrollArea->viewport() ? m_scrollArea->viewport()->width() : width();
    const int maxWidth = qMax(100, viewportWidth * kMaxBubbleWidthFraction / 100);

    const int count = m_messagesLayout->count();
    for (int i = 0; i < count; ++i) {
        QLayoutItem *item = m_messagesLayout->itemAt(i);
        if (auto *bubble = qobject_cast<ChatBubble *>(item->widget()))
            bubble->setMaxBubbleWidth(maxWidth);
    }
}

void MainWindow::scrollToBottom()
{
    QTimer::singleShot(0, this, [this]() {
        if (QScrollBar *bar = m_scrollArea->verticalScrollBar())
            bar->setValue(bar->maximum());
    });
}
