#include "HamlibRigs.h"

#include <hamlib/rig.h>

#include <algorithm>
#include <cstring>

#include <dlfcn.h>

namespace {

int collectRig(const struct rig_caps *caps, rig_ptr_t data)
{
    auto *rigs = static_cast<QList<RigInfo> *>(data);
    rigs->append({static_cast<int>(caps->rig_model),
                  QStringLiteral("%1 %2").arg(QString::fromUtf8(caps->mfg_name),
                                               QString::fromUtf8(caps->model_name))});
    return 1; // non-zero: keep iterating
}

QList<RigInfo> loadRigs()
{
    // Hamlib logs a line per backend as it loads them; mute that flood,
    // then restore the trace level HamlibLog::install() set at startup so
    // actual rig sessions afterwards are still captured in the Hamlib Log
    // window.
    rig_set_debug(RIG_DEBUG_NONE);
    rig_load_all_backends();
    rig_set_debug(RIG_DEBUG_TRACE);

    QList<RigInfo> rigs;
    rig_list_foreach(collectRig, &rigs);

    std::sort(rigs.begin(), rigs.end(), [](const RigInfo &a, const RigInfo &b) {
        return a.label.localeAwareCompare(b.label) < 0;
    });
    return rigs;
}

// Parses one of the Settings dialog's "Default" or explicit-value combo box
// choices to the enum value handshakeFor() needs, or does nothing if left
// on "Default".
enum serial_handshake_e handshakeFor(const QString &handshake, bool *ok)
{
    *ok = true;
    if (handshake == QStringLiteral("None"))
        return RIG_HANDSHAKE_NONE;
    if (handshake == QStringLiteral("XON/XOFF"))
        return RIG_HANDSHAKE_XONXOFF;
    if (handshake == QStringLiteral("Hardware"))
        return RIG_HANDSHAKE_HARDWARE;
    *ok = false;
    return RIG_HANDSHAKE_NONE;
}

// Initializes and opens a rig, applying the given connection parameters. On
// failure, returns null and fills *errorMessage; the caller owns the
// returned RIG on success and must rig_close()+rig_cleanup() it.
RIG *openRig(const RigConnectionParams &params, QString *errorMessage)
{
    RIG *rig = rig_init(params.model);
    if (!rig) {
        *errorMessage = QStringLiteral("Could not initialize this rig model.");
        return nullptr;
    }

    const QByteArray portBytes = params.port.toUtf8();
    if (!portBytes.isEmpty()) {
        std::strncpy(rig->state.rigport.pathname, portBytes.constData(),
                     sizeof(rig->state.rigport.pathname) - 1);
        rig->state.rigport.pathname[sizeof(rig->state.rigport.pathname) - 1] = '\0';
    }

    bool baudOk = false;
    const int baud = params.baudRate.toInt(&baudOk);
    if (baudOk)
        rig->state.rigport.parm.serial.rate = baud;

    bool dataBitsOk = false;
    const int dataBits = params.dataBits.toInt(&dataBitsOk);
    if (dataBitsOk)
        rig->state.rigport.parm.serial.data_bits = dataBits;

    bool stopBitsOk = false;
    const int stopBits = params.stopBits.toInt(&stopBitsOk);
    if (stopBitsOk)
        rig->state.rigport.parm.serial.stop_bits = stopBits;

    bool handshakeOk = false;
    const enum serial_handshake_e handshake = handshakeFor(params.handshake, &handshakeOk);
    if (handshakeOk)
        rig->state.rigport.parm.serial.handshake = handshake;

    const int retcode = rig_open(rig);
    if (retcode != RIG_OK) {
        *errorMessage = QStringLiteral("Connection failed: %1")
                             .arg(QString::fromUtf8(rigerror2(retcode)).trimmed());
        rig_cleanup(rig);
        return nullptr;
    }
    return rig;
}

} // namespace

const QList<RigInfo> &hamlibAvailableRigs()
{
    static const QList<RigInfo> rigs = loadRigs();
    return rigs;
}

QString hamlibVersionString()
{
    return QString::fromUtf8(hamlib_version2);
}

QString hamlibLibraryPath()
{
    Dl_info info;
    if (dladdr(reinterpret_cast<void *>(&rig_init), &info) && info.dli_fname)
        return QString::fromUtf8(info.dli_fname);
    return QStringLiteral("(unknown)");
}

RigConnectionResult connectAndQueryRig(const RigConnectionParams &params)
{
    RigConnectionResult result;

    RIG *rig = openRig(params, &result.message);
    if (!rig)
        return result;

    freq_t freq = 0;
    int retcode = rig_get_freq(rig, RIG_VFO_CURR, &freq);
    if (retcode != RIG_OK) {
        result.message = QStringLiteral("Connected, but could not read frequency: %1")
                              .arg(QString::fromUtf8(rigerror2(retcode)).trimmed());
        rig_close(rig);
        rig_cleanup(rig);
        return result;
    }

    rmode_t mode = RIG_MODE_NONE;
    pbwidth_t width = 0;
    const int modeRetcode = rig_get_mode(rig, RIG_VFO_CURR, &mode, &width);

    rig_close(rig);
    rig_cleanup(rig);

    QString modeText = QStringLiteral("unknown mode");
    if (modeRetcode == RIG_OK && mode != RIG_MODE_NONE) {
        modeText = QString::fromUtf8(rig_strrmode(mode));
        if (width > 0)
            modeText += QStringLiteral(" (%1 Hz)").arg(width);
    }

    result.success = true;
    result.message = QStringLiteral("%1 MHz  %2").arg(QString::number(freq / 1.0e6, 'f', 6), modeText);
    return result;
}

QString setRigPtt(const RigConnectionParams &params, bool on)
{
    QString errorMessage;
    RIG *rig = openRig(params, &errorMessage);
    if (!rig)
        return errorMessage;

    const int retcode = rig_set_ptt(rig, RIG_VFO_CURR, on ? RIG_PTT_ON : RIG_PTT_OFF);
    rig_close(rig);
    rig_cleanup(rig);

    if (retcode != RIG_OK) {
        return QStringLiteral("PTT %1 failed: %2")
            .arg(on ? QStringLiteral("on") : QStringLiteral("off"),
                 QString::fromUtf8(rigerror2(retcode)).trimmed());
    }
    return {};
}

QString setRigFrequency(const RigConnectionParams &params, double freqHz)
{
    QString errorMessage;
    RIG *rig = openRig(params, &errorMessage);
    if (!rig)
        return errorMessage;

    const int retcode = rig_set_freq(rig, RIG_VFO_CURR, freqHz);
    rig_close(rig);
    rig_cleanup(rig);

    if (retcode != RIG_OK) {
        return QStringLiteral("Set frequency failed: %1")
            .arg(QString::fromUtf8(rigerror2(retcode)).trimmed());
    }
    return {};
}
