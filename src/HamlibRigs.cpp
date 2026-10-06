#include "HamlibRigs.h"

#include <hamlib/rig.h>

#include <algorithm>
#include <cstring>

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
    // Hamlib logs a line per backend as it loads them; we don't want that
    // on stdout/stderr every time the app starts.
    rig_set_debug(RIG_DEBUG_NONE);
    rig_load_all_backends();

    QList<RigInfo> rigs;
    rig_list_foreach(collectRig, &rigs);

    std::sort(rigs.begin(), rigs.end(), [](const RigInfo &a, const RigInfo &b) {
        return a.label.localeAwareCompare(b.label) < 0;
    });
    return rigs;
}

} // namespace

const QList<RigInfo> &hamlibAvailableRigs()
{
    static const QList<RigInfo> rigs = loadRigs();
    return rigs;
}

RigConnectionResult connectAndQueryRig(int model, const QString &port, const QString &baudRate)
{
    RigConnectionResult result;

    RIG *rig = rig_init(model);
    if (!rig) {
        result.message = QStringLiteral("Could not initialize this rig model.");
        return result;
    }

    const QByteArray portBytes = port.toUtf8();
    if (!portBytes.isEmpty()) {
        std::strncpy(rig->state.rigport.pathname, portBytes.constData(),
                     sizeof(rig->state.rigport.pathname) - 1);
        rig->state.rigport.pathname[sizeof(rig->state.rigport.pathname) - 1] = '\0';
    }

    bool baudOk = false;
    const int baud = baudRate.toInt(&baudOk);
    if (baudOk)
        rig->state.rigport.parm.serial.rate = baud;

    int retcode = rig_open(rig);
    if (retcode != RIG_OK) {
        result.message = QStringLiteral("Connection failed: %1")
                              .arg(QString::fromUtf8(rigerror2(retcode)).trimmed());
        rig_cleanup(rig);
        return result;
    }

    freq_t freq = 0;
    retcode = rig_get_freq(rig, RIG_VFO_CURR, &freq);
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
