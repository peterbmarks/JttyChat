#include "MacroEditDialog.h"
#include "JttyCodec.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

MacroEditDialog::MacroEditDialog(const QString &title, const QString &text, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Macro"));
    setMinimumWidth(320);

    auto *mainLayout = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    m_titleEdit = new QLineEdit(title, this);
    form->addRow(tr("Button title:"), m_titleEdit);

    m_textEdit = new QLineEdit(text, this);
    m_textEdit->setMaxLength(Jtty::maxMessageLength);
    form->addRow(tr("Text to insert:"), m_textEdit);

    mainLayout->addLayout(form);

    auto *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    m_titleEdit->setFocus();
    m_titleEdit->selectAll();
}

QString MacroEditDialog::buttonTitle() const
{
    return m_titleEdit->text().trimmed();
}

QString MacroEditDialog::macroText() const
{
    return m_textEdit->text();
}
