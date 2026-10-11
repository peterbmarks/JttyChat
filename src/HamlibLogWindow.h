#pragma once

#include <QWidget>

class QPlainTextEdit;

// Non-modal viewer for Hamlib's internal debug/trace log (see HamlibLog),
// opened from File > Hamlib Log. Stays open and keeps updating live while
// the app runs; closing it just hides it, the underlying log keeps
// accumulating so reopening it shows the full history again.
class HamlibLogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit HamlibLogWindow(QWidget *parent = nullptr);

private slots:
    void appendLine(const QString &line);

private:
    QPlainTextEdit *m_textEdit;
};
