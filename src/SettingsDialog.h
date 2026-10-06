#pragma once

#include <QDialog>

class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;

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
    void connectToRig();

private:
    void loadSettings();
    void populateAudioDevices();
    void populateRigModels();
    void populateRigPorts();
    void populateRigBaudRates();

    QLineEdit *m_callsignEdit;
    QComboBox *m_audioInputCombo;
    QComboBox *m_audioOutputCombo;
    QComboBox *m_rigModelCombo;
    QComboBox *m_rigPortCombo;
    QComboBox *m_rigBaudRateCombo;
    QPushButton *m_connectButton;
    QLabel *m_rigStatusLabel;
};
