#pragma once

#include <QList>
#include <QString>

// One entry from Hamlib's rig backend registry: a model id (as used by
// rig_init()) paired with a human-readable "Manufacturer Model" label.
struct RigInfo
{
    int model;
    QString label;
};

// All transceiver models Hamlib knows how to talk to, sorted by label.
// Loads and queries Hamlib's backend registry on first call and caches
// the result for the lifetime of the process.
const QList<RigInfo> &hamlibAvailableRigs();

struct RigConnectionResult
{
    bool success = false;
    QString message; // "<freq> MHz  <mode>" on success, an error description otherwise
};

// Opens the given rig model on the given port (a serial device path, or a
// "host:port" address for the "Hamlib NET rigctl" model), reads its current
// frequency and mode, then closes it again. baudRate is one of the strings
// offered by the Settings dialog's baud rate combo box ("Default" to leave
// it at the rig backend's default). Blocks for as long as Hamlib takes to
// open the port and respond (or time out) - call this off the UI thread.
RigConnectionResult connectAndQueryRig(int model, const QString &port, const QString &baudRate);

// Opens the rig just long enough to key or unkey PTT, then closes it again.
// Meant to run off the UI thread (serial open can block briefly). Returns an
// error message, or an empty string on success.
QString setRigPtt(int model, const QString &port, const QString &baudRate, bool on);

// Opens the rig just long enough to set its current VFO frequency (in Hz),
// then closes it again. Meant to run off the UI thread. Returns an error
// message, or an empty string on success.
QString setRigFrequency(int model, const QString &port, const QString &baudRate, double freqHz);
