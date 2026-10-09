#include "ChatBubble.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QTime>
#include <QVBoxLayout>

namespace {
constexpr int kBubblePaddingV = 8;
constexpr int kBubblePaddingH = 12;
constexpr int kDefaultMaxWidth = 340;
}

ChatBubble::ChatBubble(const QString &text, bool isSent, QWidget *parent)
    : QWidget(parent)
    , m_isSent(isSent)
    , m_label(new QLabel(text, this))
    , m_timestampLabel(new QLabel(
          QLocale::system().toString(QTime::currentTime(), QLocale::ShortFormat), this))
{
    m_label->setWordWrap(true);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_label->setMaximumWidth(kDefaultMaxWidth);

    const QString background = m_isSent ? QStringLiteral("#0b93f6") : QStringLiteral("#e5e5ea");
    const QString textColor = m_isSent ? QStringLiteral("#ffffff") : QStringLiteral("#000000");

    m_label->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background-color: %1;"
        "  color: %2;"
        "  border-radius: 14px;"
        "  padding: %3px %4px;"
        "  font-size: 14px;"
        "}")
        .arg(background, textColor)
        .arg(kBubblePaddingV)
        .arg(kBubblePaddingH));

    m_timestampLabel->setStyleSheet(QStringLiteral("color: #9a9a9e; font-size: 11px;"));
    m_timestampLabel->setAlignment(m_isSent ? Qt::AlignRight : Qt::AlignLeft);

    auto *column = new QVBoxLayout;
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(2);
    column->addWidget(m_label);
    column->addWidget(m_timestampLabel);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(0);

    if (m_isSent) {
        layout->addStretch(1);
        layout->addLayout(column);
    } else {
        layout->addLayout(column);
        layout->addStretch(1);
    }
}

void ChatBubble::setMaxBubbleWidth(int width)
{
    m_label->setMaximumWidth(qMax(80, width));
    m_timestampLabel->setMaximumWidth(qMax(80, width));
}

void ChatBubble::setText(const QString &text)
{
    m_label->setText(text);
}
