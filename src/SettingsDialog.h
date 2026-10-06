#pragma once

#include <QDialog>

class QLineEdit;
class QComboBox;

// Application settings dialog. Persists its fields via QSettings and
// reloads them each time the dialog is opened, so it always reflects
// what's currently saved (including the live set of audio devices).
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void save();

private:
    void loadSettings();
    void populateAudioDevices();

    QLineEdit *m_callsignEdit;
    QComboBox *m_audioInputCombo;
    QComboBox *m_audioOutputCombo;
};
