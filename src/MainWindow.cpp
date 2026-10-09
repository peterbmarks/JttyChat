#include "MainWindow.h"
#include "AppSettingsKeys.h"
#include "AudioSpectrum.h"
#include "ChatBubble.h"
#include "HamlibRigs.h"
#include "JttyCodec.h"
#include "JttyDecoder.h"
#include "SettingsDialog.h"
#include "SpectrumWidget.h"
#include "TextCapitalization.h"

#include <QAction>
#include <QApplication>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QBuffer>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMediaDevices>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

#include <cstring>
#include <thread>

namespace {
constexpr int kMaxBubbleWidthFraction = 70; // percent of viewport width
constexpr int kPttLeadMs = 150; // brief key-up lead before audio starts, for real radios
constexpr int kSpectrumLowHz = 1400;
constexpr int kSpectrumHighHz = 1700;
constexpr int kSpectrumFftSize = 8192; // ~1.5 Hz/bin at the 12 kHz Rx rate
constexpr int kTxTailMs = 200;  // margin after audio ends before unkeying/re-enabling Send
constexpr int kDecodeStatusIdleMs = 5000; // clear the SNR/error line this long after a decode

QAudioDevice findAudioDevice(const QList<QAudioDevice> &devices, const QByteArray &id,
                              const QAudioDevice &fallback)
{
    for (const QAudioDevice &device : devices) {
        if (device.id() == id)
            return device;
    }
    return fallback;
}

struct RigTxSettings
{
    bool configured = false;
    int model = 0;
    QString port;
    QString baudRate;
};

RigTxSettings loadRigTxSettings()
{
    QSettings settings;
    settings.beginGroup(SettingsKeys::transceiverGroup);
    RigTxSettings rig;
    const bool haveModel = settings.contains(SettingsKeys::rigModel);
    rig.model = settings.value(SettingsKeys::rigModel).toInt();
    rig.port = settings.value(SettingsKeys::rigPort).toString().trimmed();
    rig.baudRate = settings.value(SettingsKeys::rigBaudRate).toString();
    settings.endGroup();
    rig.configured = haveModel && !rig.port.isEmpty();
    return rig;
}

// If "Append callsign" is enabled and a callsign is configured, appends
// "-CALLSIGN" to text, but only when it still fits within the JTTY message
// length limit; otherwise returns text unchanged.
QString withCallsignAppended(const QString &text)
{
    QSettings settings;
    const bool appendCallsign = settings.value(SettingsKeys::appendCallsign, false).toBool();
    const QString callsign = settings.value(SettingsKeys::callsign).toString().trimmed();
    if (!appendCallsign || callsign.isEmpty())
        return text;

    const QString suffix = QStringLiteral("-%1").arg(callsign.toUpper());
    if (text.length() + suffix.length() > Jtty::maxMessageLength)
        return text;

    return text + suffix;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("JttyChat"));
    resize(420, 640);

    createMenuBar();

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Live spectrum around the JTTY tone, from the receive audio.
    m_spectrumWidget = new SpectrumWidget(kSpectrumLowHz, kSpectrumHighHz, central);
    rootLayout->addWidget(m_spectrumWidget, 0);

    // SNR / error count of the most recent decode, cleared a while after
    // decoding goes quiet (see m_decodeStatusClearTimer).
    m_decodeStatusLabel = new QLabel(central);
    m_decodeStatusLabel->setStyleSheet(
        QStringLiteral("color: #6e6e73; font-size: 11px; padding: 2px 8px;"));
    rootLayout->addWidget(m_decodeStatusLabel, 0);

    m_decodeStatusClearTimer = new QTimer(this);
    m_decodeStatusClearTimer->setSingleShot(true);
    m_decodeStatusClearTimer->setInterval(kDecodeStatusIdleMs);
    connect(m_decodeStatusClearTimer, &QTimer::timeout, m_decodeStatusLabel, &QLabel::clear);

    // Scrolling message history.
    m_scrollArea = new QScrollArea(central);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(QStringLiteral("background-color: #ffffff;"));

    m_messagesContainer = new QWidget(m_scrollArea);
    m_messagesContainer->setStyleSheet(QStringLiteral("background-color: #ffffff;"));
    m_messagesLayout = new QVBoxLayout(m_messagesContainer);
    m_messagesLayout->setContentsMargins(4, 8, 4, 8);
    m_messagesLayout->setSpacing(4);
    m_messagesLayout->addStretch(1);

    m_scrollArea->setWidget(m_messagesContainer);
    rootLayout->addWidget(m_scrollArea, 1);

    // The content height (and so the scrollbar's range) only settles once
    // Qt actually reflows the newly added bubbles, which happens after
    // addMessage() returns. Scrolling here, exactly when the range catches
    // up, is reliable regardless of how many messages land in one burst -
    // unlike guessing at a fixed delay.
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, this,
            [this](int, int max) {
                if (m_pendingScrollToBottom)
                    m_scrollArea->verticalScrollBar()->setValue(max);
            });

    // Bottom input bar.
    auto *inputBar = new QWidget(central);
    inputBar->setStyleSheet(QStringLiteral("background-color: #f2f2f2;"));
    auto *inputLayout = new QHBoxLayout(inputBar);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(8);

    m_inputField = new QLineEdit(inputBar);
    m_inputField->setPlaceholderText(QStringLiteral("Text Message"));
    m_inputField->setMaxLength(Jtty::maxMessageLength);
    m_inputField->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  border: 1px solid #c8c8c8;"
        "  border-radius: 16px;"
        "  padding: 8px 14px;"
        "  font-size: 14px;"
        "  background-color: #ffffff;"
        "}"));

    m_sendButton = new QPushButton(QStringLiteral("Send"), inputBar);
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_sendButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #0b93f6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 16px;"
        "  padding: 8px 18px;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #a8d4fb;"
        "}"
        "QPushButton:hover:!disabled {"
        "  background-color: #0a84dd;"
        "}"));

    inputLayout->addWidget(m_inputField, 1);
    inputLayout->addWidget(m_sendButton, 0);

    rootLayout->addWidget(inputBar, 0);

    setCentralWidget(central);

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(m_inputField, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);
    m_inputField->installEventFilter(this);

    m_inputField->setFocus();

    m_jttyDecoder = new JttyDecoder(this);
    connect(m_jttyDecoder, &JttyDecoder::messageUpdated, this, &MainWindow::onJttyMessageUpdated);

    m_audioSpectrum = new AudioSpectrum(kSpectrumFftSize, Jtty::rxSampleRate);

    startJttyReceiver();
}

MainWindow::~MainWindow()
{
    stopJttyReceiver();
    delete m_audioSpectrum;
}

void MainWindow::createMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

    QAction *settingsAction = fileMenu->addAction(QStringLiteral("&Settings..."));
    settingsAction->setMenuRole(QAction::PreferencesRole);
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettingsDialog);

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction(QStringLiteral("&Quit"));
    quitAction->setMenuRole(QAction::QuitRole);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu *freqMenu = menuBar()->addMenu(QStringLiteral("Fre&quency"));
    static constexpr double kBandFrequenciesMHz[] = {1.838, 3.575, 7.090,  10.140, 14.090,
                                                       18.100, 21.090, 24.920, 28.090};
    for (double freqMHz : kBandFrequenciesMHz) {
        QAction *freqAction =
            freqMenu->addAction(QStringLiteral("%1 MHz").arg(freqMHz, 0, 'f', 3));
        connect(freqAction, &QAction::triggered, this,
                [this, freqMHz]() { tuneRigToFrequency(freqMHz); });
    }
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Audio device choices may have changed; restart capture against
        // whatever is now configured.
        startJttyReceiver();
    }
}

void MainWindow::sendMessage()
{
    const QString typedText = m_inputField->text().trimmed();
    if (typedText.isEmpty())
        return;

    const QString text = withCallsignAppended(typedText);

    const Jtty::EncodedMessage encoded = Jtty::encodeMessage(text, Jtty::defaultToneHz);
    if (!encoded.ok) {
        QMessageBox::warning(this, tr("JTTY"),
            tr("This message couldn't be encoded for JTTY (it may need more than "
               "16 frames' worth of compact atoms to send)."));
        return;
    }

    m_lastSentMessage = typedText;

    addMessage(encoded.canonicalText, true);
    m_inputField->clear();

    transmitJtty(encoded.samples);
}

void MainWindow::onJttyMessageUpdated(qint64 messageId, QString text, float frequencyHz,
                                       float snrDb, int errorCount, bool complete)
{
    Q_UNUSED(frequencyHz);

    m_decodeStatusLabel->setText(
        tr("SNR: %1 dB   Errors: %2").arg(qRound(snrDb)).arg(errorCount));
    m_decodeStatusClearTimer->start();

    auto pending = m_pendingReceivedBubbles.find(messageId);
    if (pending != m_pendingReceivedBubbles.end()) {
        if (ChatBubble *bubble = *pending) {
            bubble->setText(formatForDisplay(text));
            updateBubbleWidths();
            scrollToBottom();
        }
        if (complete)
            m_pendingReceivedBubbles.erase(pending);
        return;
    }

    ChatBubble *bubble = createBubble(text, false);
    if (!complete)
        m_pendingReceivedBubbles.insert(messageId, bubble);
}

void MainWindow::transmitJtty(const QVector<int16_t> &samples)
{
    if (samples.isEmpty())
        return;

    m_inputField->setEnabled(false);
    m_sendButton->setEnabled(false);

    const RigTxSettings rig = loadRigTxSettings();
    if (rig.configured) {
        std::thread([rig]() { setRigPtt(rig.model, rig.port, rig.baudRate, true); }).detach();
    }

    const int leadMs = rig.configured ? kPttLeadMs : 0;
    QTimer::singleShot(leadMs, this, [this, samples]() {
        QSettings settings;
        settings.beginGroup(SettingsKeys::transceiverGroup);
        const QByteArray savedOutputId = settings.value(SettingsKeys::audioOutputDeviceId).toByteArray();
        settings.endGroup();

        const QAudioDevice device = findAudioDevice(
            QMediaDevices::audioOutputs(), savedOutputId, QMediaDevices::defaultAudioOutput());
        if (device.isNull())
            return;

        QAudioFormat format;
        format.setSampleRate(Jtty::txSampleRate);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);

        delete m_audioSink;
        delete m_txBuffer;

        QByteArray bytes(reinterpret_cast<const char *>(samples.constData()),
                          samples.size() * int(sizeof(int16_t)));
        m_txBuffer = new QBuffer(this);
        m_txBuffer->setData(bytes);
        m_txBuffer->open(QIODevice::ReadOnly);

        m_audioSink = new QAudioSink(device, format, this);
        m_audioSink->start(m_txBuffer);
    });

    const qint64 durationMs = qint64(samples.size()) * 1000 / Jtty::txSampleRate;
    const int totalMs = int(leadMs + durationMs + kTxTailMs);

    QTimer::singleShot(totalMs, this, [this, rig]() {
        if (rig.configured) {
            std::thread([rig]() { setRigPtt(rig.model, rig.port, rig.baudRate, false); }).detach();
        }
        m_inputField->setEnabled(true);
        m_sendButton->setEnabled(true);
        m_inputField->setFocus();
    });
}

void MainWindow::tuneRigToFrequency(double freqMHz)
{
    const RigTxSettings rig = loadRigTxSettings();
    if (!rig.configured) {
        QMessageBox::warning(this, tr("Frequency"),
                              tr("Configure a rig model and port in Settings first."));
        return;
    }

    std::thread([this, rig, freqMHz]() {
        const QString error = setRigFrequency(rig.model, rig.port, rig.baudRate, freqMHz * 1.0e6);
        if (!error.isEmpty()) {
            QMetaObject::invokeMethod(
                this, [this, error]() { QMessageBox::warning(this, tr("Frequency"), error); },
                Qt::QueuedConnection);
        }
    }).detach();
}

void MainWindow::startJttyReceiver()
{
    stopJttyReceiver();

    QSettings settings;
    settings.beginGroup(SettingsKeys::transceiverGroup);
    const QByteArray savedInputId = settings.value(SettingsKeys::audioInputDeviceId).toByteArray();
    settings.endGroup();

    const QAudioDevice device = findAudioDevice(
        QMediaDevices::audioInputs(), savedInputId, QMediaDevices::defaultAudioInput());
    if (device.isNull())
        return;

    QAudioFormat format;
    format.setSampleRate(Jtty::rxSampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    m_audioSource = new QAudioSource(device, format, this);
    m_audioSourceDevice = m_audioSource->start();
    if (!m_audioSourceDevice) {
        delete m_audioSource;
        m_audioSource = nullptr;
        return;
    }

    connect(m_audioSourceDevice, &QIODevice::readyRead, this, [this]() {
        const QByteArray chunk = m_audioSourceDevice->readAll();
        const int sampleCount = chunk.size() / int(sizeof(int16_t));
        if (sampleCount <= 0)
            return;

        QVector<int16_t> samples(sampleCount);
        std::memcpy(samples.data(), chunk.constData(), sampleCount * sizeof(int16_t));

        QVector<float> magnitudesDb;
        if (m_audioSpectrum->addSamples(samples, kSpectrumLowHz, kSpectrumHighHz, magnitudesDb))
            m_spectrumWidget->setMagnitudesDb(magnitudesDb);

        m_jttyDecoder->addSamples(samples);
        m_jttyDecoder->poll();
    });
}

void MainWindow::stopJttyReceiver()
{
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_audioSourceDevice = nullptr;
    }
}

void MainWindow::addMessage(const QString &text, bool isSent)
{
    createBubble(text, isSent);
}

ChatBubble *MainWindow::createBubble(const QString &text, bool isSent)
{
    auto *bubble = new ChatBubble(formatForDisplay(text), isSent, m_messagesContainer);

    // Insert before the trailing stretch so new bubbles land at the bottom
    // of the scroll area while older ones stay pinned to the top.
    const int insertIndex = m_messagesLayout->count() - 1;
    m_messagesLayout->insertWidget(insertIndex, bubble);

    updateBubbleWidths();
    scrollToBottom();
    return bubble;
}

QString MainWindow::formatForDisplay(const QString &text) const
{
    QSettings settings;
    if (!settings.value(SettingsKeys::capitalizeText, false).toBool())
        return text;
    return TextCapitalization::apply(text);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateBubbleWidths();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_inputField && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Up && !m_lastSentMessage.isEmpty()) {
            m_inputField->setText(m_lastSentMessage);
            m_inputField->selectAll();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::updateBubbleWidths()
{
    const int viewportWidth = m_scrollArea->viewport() ? m_scrollArea->viewport()->width() : width();
    const int maxWidth = qMax(100, viewportWidth * kMaxBubbleWidthFraction / 100);

    const int count = m_messagesLayout->count();
    for (int i = 0; i < count; ++i) {
        QLayoutItem *item = m_messagesLayout->itemAt(i);
        if (auto *bubble = qobject_cast<ChatBubble *>(item->widget()))
            bubble->setMaxBubbleWidth(maxWidth);
    }
}

void MainWindow::scrollToBottom()
{
    // Best-effort immediate scroll (correct if the range is already
    // current), backstopped by the rangeChanged handler above for once the
    // new bubble's layout actually settles.
    m_pendingScrollToBottom = true;
    if (QScrollBar *bar = m_scrollArea->verticalScrollBar())
        bar->setValue(bar->maximum());
}
