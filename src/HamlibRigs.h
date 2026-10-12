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

// Hamlib's own version string (e.g. "Hamlib 4.6.5").
QString hamlibVersionString();

// Filesystem path of the libhamlib shared object actually loaded at
// runtime, as resolved by the dynamic linker (distinct from the headers
// it was built against, if more than one Hamlib is installed).
QString hamlibLibraryPath();

struct RigConnectionResult
{
    bool success = false;
    QString message; // "<freq> MHz  <mode>" on success, an error description otherwise
};

// Everything needed to open a rig. port is a serial device path, or a
// "host:port" address for the "Hamlib NET rigctl" model. baudRate,
// dataBits, stopBits and handshake are each one of the strings offered by
// the matching Settings dialog combo box - "Default" leaves that aspect of
// the connection at the rig backend's own default instead of overriding it.
struct RigConnectionParams
{
    int model = 0;
    QString port;
    QString baudRate;
    QString dataBits;
    QString stopBits;
    QString handshake;
};

// Opens the rig, reads its current frequency and mode, then closes it
// again. Blocks for as long as Hamlib takes to open the port and respond
// (or time out) - call this off the UI thread.
RigConnectionResult connectAndQueryRig(const RigConnectionParams &params);

// Opens the rig just long enough to key or unkey PTT, then closes it again.
// Meant to run off the UI thread (serial open can block briefly). Returns an
// error message, or an empty string on success.
QString setRigPtt(const RigConnectionParams &params, bool on);

// Opens the rig just long enough to set its current VFO frequency (in Hz),
// then closes it again. Meant to run off the UI thread. Returns an error
// message, or an empty string on success.
QString setRigFrequency(const RigConnectionParams &params, double freqHz);
