#pragma once

#include <QMutex>
#include <QObject>
#include <QStringList>

// Captures Hamlib's internal debug/trace log stream, which otherwise just
// goes to stderr, via rig_set_debug_callback() (see HamlibLog.cpp). Keeps a
// bounded history so HamlibLogWindow can show everything captured so far
// when first opened, and emits lineAdded() for everything captured after
// that so the window stays live.
//
// Hamlib can invoke the debug callback from any thread (rig I/O runs off
// the UI thread elsewhere in this app - see HamlibRigs.cpp), so append() is
// safe to call from any thread; lineAdded() still reaches the GUI thread
// safely because Qt upgrades the connection to queued automatically when
// the receiver (HamlibLogWindow) lives on a different thread than the
// emitting call.
class HamlibLog : public QObject
{
    Q_OBJECT

public:
    static HamlibLog &instance();

    // Installs the Hamlib debug callback and raises the trace level high
    // enough to capture actual CAT command bytes, not just open/close
    // events. Call once at startup.
    void install();

    void append(const QString &line);
    QString text() const;

signals:
    void lineAdded(const QString &line);

private:
    HamlibLog() = default;

    mutable QMutex m_mutex;
    QStringList m_lines;
    static constexpr int kMaxLines = 5000;
};
