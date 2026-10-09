#pragma once

#include <QHash>
#include <QMainWindow>
#include <QPointer>

#include <cstdint>

class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QTimer;
class QVBoxLayout;
class QAudioSource;
class QAudioSink;
class QBuffer;
class QIODevice;
class JttyDecoder;
class SpectrumWidget;
class AudioSpectrum;
class ChatBubble;

// Top-level window: a scrolling column of speech-bubble messages above
// a text field + Send button, in the style of a simple iMessage-like client.
// Sending transmits over JTTY (see src/JttyCodec.h); received JTTY messages
// appear as incoming bubbles via a continuously running JttyDecoder.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    // Appends a message bubble. isSent = true renders it as an outgoing
    // (right-aligned, blue) bubble; false renders it as incoming
    // (left-aligned, grey) bubble.
    void addMessage(const QString &text, bool isSent);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void sendMessage();
    void openSettingsDialog();
    void onJttyMessageUpdated(qint64 messageId, QString text, float frequencyHz, float snrDb,
                               int errorCount, bool complete);

private:
    void createMenuBar();
    ChatBubble *createBubble(const QString &text, bool isSent);
    void updateBubbleWidths();
    void scrollToBottom();

    void startJttyReceiver();
    void stopJttyReceiver();
    void transmitJtty(const QVector<int16_t> &samples);
    void tuneRigToFrequency(double freqMHz);

    QScrollArea *m_scrollArea;
    QWidget *m_messagesContainer;
    QVBoxLayout *m_messagesLayout;
    QLineEdit *m_inputField;
    QPushButton *m_sendButton;

    JttyDecoder *m_jttyDecoder;
    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_audioSourceDevice = nullptr;

    QAudioSink *m_audioSink = nullptr;
    QBuffer *m_txBuffer = nullptr;

    SpectrumWidget *m_spectrumWidget;
    AudioSpectrum *m_audioSpectrum;
    QLabel *m_decodeStatusLabel;
    QTimer *m_decodeStatusClearTimer;

    bool m_pendingScrollToBottom = false;

    // What the user typed into m_inputField last time Send was used,
    // restored when they press Up in an empty-ish field (see eventFilter).
    QString m_lastSentMessage;

    // Bubbles for JTTY messages still being received, keyed by message ID,
    // so later frames of the same message update the existing bubble
    // instead of creating a new one. Erased once a message completes.
    QHash<qint64, QPointer<ChatBubble>> m_pendingReceivedBubbles;
};
