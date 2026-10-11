#include "HamlibLog.h"

#include <hamlib/rig.h>

#include <cstdarg>
#include <cstdio>

namespace {

QString levelTag(enum rig_debug_level_e level)
{
    switch (level) {
    case RIG_DEBUG_BUG:
        return QStringLiteral("[BUG] ");
    case RIG_DEBUG_ERR:
        return QStringLiteral("[ERROR] ");
    case RIG_DEBUG_WARN:
        return QStringLiteral("[WARN] ");
    case RIG_DEBUG_VERBOSE:
        return QStringLiteral("[VERBOSE] ");
    case RIG_DEBUG_TRACE:
        return QStringLiteral("[TRACE] ");
    case RIG_DEBUG_CACHE:
        return QStringLiteral("[CACHE] ");
    default:
        return QString();
    }
}

// Installed via rig_set_debug_callback(); replaces Hamlib's default
// stderr output entirely, so this is the only place its log messages go.
int hamlibDebugCallback(enum rig_debug_level_e level, rig_ptr_t, const char *fmt, va_list ap)
{
    char buffer[2048];
    std::vsnprintf(buffer, sizeof(buffer), fmt, ap);

    const QString line = QString::fromUtf8(buffer).trimmed();
    if (!line.isEmpty())
        HamlibLog::instance().append(levelTag(level) + line);
    return RIG_OK;
}

} // namespace

HamlibLog &HamlibLog::instance()
{
    static HamlibLog log;
    return log;
}

void HamlibLog::install()
{
    rig_set_debug_callback(&hamlibDebugCallback, nullptr);
    rig_set_debug(RIG_DEBUG_TRACE);
}

void HamlibLog::append(const QString &line)
{
    {
        QMutexLocker locker(&m_mutex);
        m_lines.append(line);
        while (m_lines.size() > kMaxLines)
            m_lines.removeFirst();
    }
    emit lineAdded(line);
}

QString HamlibLog::text() const
{
    QMutexLocker locker(&m_mutex);
    return m_lines.join(QLatin1Char('\n'));
}
