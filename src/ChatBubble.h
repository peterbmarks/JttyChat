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

    // Replaces the bubble's text in place, e.g. as a JTTY message decodes
    // frame by frame.
    void setText(const QString &text);

private:
    bool m_isSent;
    QLabel *m_label;
    QLabel *m_timestampLabel;
};
