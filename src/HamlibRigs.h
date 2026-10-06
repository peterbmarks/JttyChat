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
