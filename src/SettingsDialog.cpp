#include "SettingsDialog.h"
#include "AppSettingsKeys.h"
#include "HamlibRigs.h"

#include <QAudioDevice>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMediaDevices>
#include <QMetaObject>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include <thread>

namespace {
constexpr auto kDefaultBaudRate = "Default";

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

    m_appendCallsignCheck = new QCheckBox(QStringLiteral("Append callsign"), this);
    m_appendCallsignCheck->setToolTip(
        QStringLiteral("When sending, append \"-CALLSIGN\" to the message if it fits."));
    generalForm->addRow(QString(), m_appendCallsignCheck);

    mainLayout->addLayout(generalForm);

    auto *transceiverGroup = new QGroupBox(QStringLiteral("Transceiver"), this);
    auto *transceiverForm = new QFormLayout(transceiverGroup);

    m_audioInputCombo = new QComboBox(transceiverGroup);
    transceiverForm->addRow(QStringLiteral("Audio Input:"), m_audioInputCombo);

    m_audioOutputCombo = new QComboBox(transceiverGroup);
    transceiverForm->addRow(QStringLiteral("Audio Output:"), m_audioOutputCombo);

    // CAT control (Hamlib): which rig, and how to reach it.
    m_rigModelCombo = new QComboBox(transceiverGroup);
    m_rigModelCombo->setEditable(true);
    m_rigModelCombo->setInsertPolicy(QComboBox::NoInsert);
    transceiverForm->addRow(QStringLiteral("Rig Model:"), m_rigModelCombo);

    m_rigPortCombo = new QComboBox(transceiverGroup);
    m_rigPortCombo->setEditable(true);
    m_rigPortCombo->setInsertPolicy(QComboBox::NoInsert);
    transceiverForm->addRow(QStringLiteral("Rig Port:"), m_rigPortCombo);

    m_rigBaudRateCombo = new QComboBox(transceiverGroup);
    transceiverForm->addRow(QStringLiteral("Rig Baud Rate:"), m_rigBaudRateCombo);

    m_connectButton = new QPushButton(QStringLiteral("Connect"), transceiverGroup);
    transceiverForm->addRow(QString(), m_connectButton);
    connect(m_connectButton, &QPushButton::clicked, this, &SettingsDialog::connectToRig);

    m_rigStatusLabel = new QLabel(transceiverGroup);
    m_rigStatusLabel->setWordWrap(true);
    transceiverForm->addRow(QString(), m_rigStatusLabel);

    mainLayout->addWidget(transceiverGroup);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    populateAudioDevices();
    populateRigModels();
    populateRigPorts();
    populateRigBaudRates();
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

void SettingsDialog::populateRigModels()
{
    m_rigModelCombo->clear();
    for (const RigInfo &rig : hamlibAvailableRigs())
        m_rigModelCombo->addItem(rig.label, rig.model);

    // The 300+ Hamlib rig list is easier to use as a type-to-filter box
    // than a plain dropdown; match anywhere in the name, not just the start.
    if (QCompleter *completer = m_rigModelCombo->completer()) {
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
    }
}

void SettingsDialog::populateRigPorts()
{
    m_rigPortCombo->clear();

    QDir devDir(QStringLiteral("/dev"));
    const QStringList filters{QStringLiteral("ttyUSB*"), QStringLiteral("ttyACM*"), QStringLiteral("ttyS*")};
    for (const QString &name : devDir.entryList(filters, QDir::System, QDir::Name))
        m_rigPortCombo->addItem(devDir.filePath(name));

    // Not a serial device: rigctld (hamlib's network rig daemon), for the
    // "Hamlib NET rigctl" model.
    m_rigPortCombo->addItem(QStringLiteral("localhost:4532"));
}

void SettingsDialog::populateRigBaudRates()
{
    m_rigBaudRateCombo->clear();
    m_rigBaudRateCombo->addItem(QString::fromLatin1(kDefaultBaudRate));
    for (const char *rate : {"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"})
        m_rigBaudRateCombo->addItem(QString::fromLatin1(rate));
}

void SettingsDialog::loadSettings()
{
    QSettings settings;

    m_callsignEdit->setText(settings.value(SettingsKeys::callsign).toString());
    m_appendCallsignCheck->setChecked(settings.value(SettingsKeys::appendCallsign, false).toBool());

    settings.beginGroup(SettingsKeys::transceiverGroup);
    const QByteArray savedInputId = settings.value(SettingsKeys::audioInputDeviceId).toByteArray();
    const QByteArray savedOutputId = settings.value(SettingsKeys::audioOutputDeviceId).toByteArray();
    const bool haveSavedRigModel = settings.contains(SettingsKeys::rigModel);
    const int savedRigModel = settings.value(SettingsKeys::rigModel).toInt();
    const QString savedRigPort = settings.value(SettingsKeys::rigPort).toString();
    const QString savedBaudRate = settings.value(SettingsKeys::rigBaudRate).toString();
    settings.endGroup();

    selectDevice(m_audioInputCombo, savedInputId, QMediaDevices::defaultAudioInput());
    selectDevice(m_audioOutputCombo, savedOutputId, QMediaDevices::defaultAudioOutput());

    int rigModelIndex = haveSavedRigModel ? m_rigModelCombo->findData(savedRigModel) : -1;
    m_rigModelCombo->setCurrentIndex(rigModelIndex < 0 ? 0 : rigModelIndex);

    m_rigPortCombo->setCurrentText(savedRigPort);

    const int baudIndex = m_rigBaudRateCombo->findText(savedBaudRate);
    m_rigBaudRateCombo->setCurrentIndex(baudIndex < 0 ? 0 : baudIndex);
}

void SettingsDialog::connectToRig()
{
    if (m_rigModelCombo->currentData().isNull()) {
        m_rigStatusLabel->setStyleSheet(QStringLiteral("color: #c0392b;"));
        m_rigStatusLabel->setText(QStringLiteral("Select a rig model first."));
        return;
    }

    const int model = m_rigModelCombo->currentData().toInt();
    const QString port = m_rigPortCombo->currentText().trimmed();
    const QString baudRate = m_rigBaudRateCombo->currentText();

    m_connectButton->setEnabled(false);
    m_rigStatusLabel->setStyleSheet(QString());
    m_rigStatusLabel->setText(QStringLiteral("Connecting..."));

    // Opening a serial port can block for a while (Hamlib retries reads
    // before giving up), so do it off the UI thread and post the result
    // back. Using `this` as the invokeMethod context means the queued call
    // is silently dropped if the dialog is closed before it runs.
    std::thread([this, model, port, baudRate]() {
        const RigConnectionResult result = connectAndQueryRig(model, port, baudRate);
        QMetaObject::invokeMethod(this, [this, result]() {
            m_rigStatusLabel->setStyleSheet(result.success ? QString() : QStringLiteral("color: #c0392b;"));
            m_rigStatusLabel->setText(result.message);
            m_connectButton->setEnabled(true);
        }, Qt::QueuedConnection);
    }).detach();
}

void SettingsDialog::save()
{
    QSettings settings;

    settings.setValue(SettingsKeys::callsign, m_callsignEdit->text().trimmed());
    settings.setValue(SettingsKeys::appendCallsign, m_appendCallsignCheck->isChecked());

    settings.beginGroup(SettingsKeys::transceiverGroup);
    settings.setValue(SettingsKeys::audioInputDeviceId, m_audioInputCombo->currentData().toByteArray());
    settings.setValue(SettingsKeys::audioOutputDeviceId, m_audioOutputCombo->currentData().toByteArray());
    settings.setValue(SettingsKeys::rigModel, m_rigModelCombo->currentData().toInt());
    settings.setValue(SettingsKeys::rigPort, m_rigPortCombo->currentText().trimmed());
    settings.setValue(SettingsKeys::rigBaudRate, m_rigBaudRateCombo->currentText());
    settings.endGroup();

    accept();
}
