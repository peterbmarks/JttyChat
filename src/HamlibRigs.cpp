#include "HamlibRigs.h"

#include <hamlib/rig.h>

#include <algorithm>

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
