#include "SettingsDialog.h"

#include <QAudioDevice>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMediaDevices>
#include <QSettings>
#include <QVBoxLayout>

namespace {
constexpr auto kCallsignKey = "callsign";
constexpr auto kTransceiverGroup = "Transceiver";
constexpr auto kAudioInputKey = "audioInputDeviceId";
constexpr auto kAudioOutputKey = "audioOutputDeviceId";

// Selects the combo box entry whose stored device id matches savedId,
// falling back to the system default device if there's no saved id (or
// the saved device is no longer present, e.g. it was unplugged).
void selectDevice(QComboBox *combo, const QByteArray &savedId, const QAudioDevice &defaultDevice)
{
    int index = savedId.isEmpty() ? -1 : combo->findData(QVariant::fromValue(savedId));
    if (index < 0)
        index = combo->findData(QVariant::fromValue(defaultDevice.id()));
    if (index < 0)
        index = 0;
    combo->setCurrentIndex(index);
}
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Settings"));
    setMinimumWidth(360);

    auto *mainLayout = new QVBoxLayout(this);

    auto *generalForm = new QFormLayout;
    m_callsignEdit = new QLineEdit(this);
    m_callsignEdit->setPlaceholderText(QStringLiteral("e.g. W1AW"));
    generalForm->addRow(QStringLiteral("Callsign:"), m_callsignEdit);
    mainLayout->addLayout(generalForm);

    auto *transceiverGroup = new QGroupBox(QStringLiteral("Transceiver"), this);
    auto *transceiverForm = new QFormLayout(transceiverGroup);

    m_audioInputCombo = new QComboBox(transceiverGroup);
    transceiverForm->addRow(QStringLiteral("Audio Input:"), m_audioInputCombo);

    m_audioOutputCombo = new QComboBox(transceiverGroup);
    transceiverForm->addRow(QStringLiteral("Audio Output:"), m_audioOutputCombo);

    mainLayout->addWidget(transceiverGroup);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    populateAudioDevices();
    loadSettings();
}

void SettingsDialog::populateAudioDevices()
{
    m_audioInputCombo->clear();
    for (const QAudioDevice &device : QMediaDevices::audioInputs())
        m_audioInputCombo->addItem(device.description(), QVariant::fromValue(device.id()));

    m_audioOutputCombo->clear();
    for (const QAudioDevice &device : QMediaDevices::audioOutputs())
        m_audioOutputCombo->addItem(device.description(), QVariant::fromValue(device.id()));
}

void SettingsDialog::loadSettings()
{
    QSettings settings;

    m_callsignEdit->setText(settings.value(kCallsignKey).toString());

    settings.beginGroup(kTransceiverGroup);
    const QByteArray savedInputId = settings.value(kAudioInputKey).toByteArray();
    const QByteArray savedOutputId = settings.value(kAudioOutputKey).toByteArray();
    settings.endGroup();

    selectDevice(m_audioInputCombo, savedInputId, QMediaDevices::defaultAudioInput());
    selectDevice(m_audioOutputCombo, savedOutputId, QMediaDevices::defaultAudioOutput());
}

void SettingsDialog::save()
{
    QSettings settings;

    settings.setValue(kCallsignKey, m_callsignEdit->text().trimmed());

    settings.beginGroup(kTransceiverGroup);
    settings.setValue(kAudioInputKey, m_audioInputCombo->currentData().toByteArray());
    settings.setValue(kAudioOutputKey, m_audioOutputCombo->currentData().toByteArray());
    settings.endGroup();

    accept();
}
