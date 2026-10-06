#pragma once

#include <QWidget>

class QLabel;

// A single chat message rendered as a rounded speech bubble,
// aligned to the right for sent messages and to the left for received ones.
class ChatBubble : public QWidget
{
    Q_OBJECT

public:
    explicit ChatBubble(const QString &text, bool isSent, QWidget *parent = nullptr);

    bool isSent() const { return m_isSent; }

    // Keeps bubbles from growing wider than a sensible fraction of the
    // scroll area as the window is resized.
    void setMaxBubbleWidth(int width);

private:
    bool m_isSent;
    QLabel *m_label;
};
