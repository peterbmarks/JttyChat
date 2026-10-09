#pragma once

#include <QDialog>

class QLineEdit;

// Small dialog for editing one macro button's title and the text it
// inserts into the message field. Opened by right-clicking a macro button.
class MacroEditDialog : public QDialog
{
    Q_OBJECT

public:
    MacroEditDialog(const QString &title, const QString &text, QWidget *parent = nullptr);

    QString buttonTitle() const;
    QString macroText() const;

private:
    QLineEdit *m_titleEdit;
    QLineEdit *m_textEdit;
};
