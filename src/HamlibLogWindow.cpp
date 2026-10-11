#include "HamlibLogWindow.h"
#include "HamlibLog.h"

#include <QFont>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>

HamlibLogWindow::HamlibLogWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QStringLiteral("Hamlib Log"));
    resize(700, 450);

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_textEdit->setFont(QFont(QStringLiteral("monospace")));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_textEdit);

    connect(&HamlibLog::instance(), &HamlibLog::lineAdded, this, &HamlibLogWindow::appendLine);

    const QString history = HamlibLog::instance().text();
    if (!history.isEmpty()) {
        for (const QString &line : history.split(QLatin1Char('\n')))
            appendLine(line);
    }
}

void HamlibLogWindow::appendLine(const QString &line)
{
    // Error/bug lines are the ones worth spotting at a glance in an
    // otherwise long trace of routine CAT command bytes.
    const bool isError =
        line.startsWith(QStringLiteral("[ERROR")) || line.startsWith(QStringLiteral("[BUG"));

    QTextCharFormat format;
    if (isError)
        format.setForeground(QColor(0xc0, 0x39, 0x2b));

    QTextCursor cursor(m_textEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(line + QStringLiteral("\n"), format);

    QScrollBar *scrollBar = m_textEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
