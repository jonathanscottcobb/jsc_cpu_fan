#include "SensorModel.h"

#include <algorithm>

namespace {

bool looksLikePackageLabel(const QString &label)
{
    const QString lower = label.toLower();
    return lower.contains(QStringLiteral("package"))
        || lower.contains(QStringLiteral("tctl"))
        || lower == QStringLiteral("tccd1")
        || lower.contains(QStringLiteral("cpu"));
}

bool looksLikeCoreLabel(const QString &label)
{
    const QString lower = label.toLower();
    return lower.startsWith(QStringLiteral("core "))
        || lower.contains(QStringLiteral("core"));
}

} // namespace

std::optional<double> SensorSnapshot::primaryCpuCelsius() const
{
    for (const TempReading &t : temps) {
        if (looksLikePackageLabel(t.label)) {
            return t.celsius;
        }
    }

    double maxCore = -1.0;
    bool foundCore = false;
    for (const TempReading &t : temps) {
        if (looksLikeCoreLabel(t.label)) {
            maxCore = std::max(maxCore, t.celsius);
            foundCore = true;
        }
    }
    if (foundCore) {
        return maxCore;
    }

    if (!temps.isEmpty()) {
        double maxAll = temps.first().celsius;
        for (const TempReading &t : temps) {
            maxAll = std::max(maxAll, t.celsius);
        }
        return maxAll;
    }

    return std::nullopt;
}

bool SensorSnapshot::cpuIsHot(double warnCelsius) const
{
    const auto primary = primaryCpuCelsius();
    if (!primary.has_value()) {
        return false;
    }

    for (const TempReading &t : temps) {
        if (looksLikePackageLabel(t.label) || looksLikeCoreLabel(t.label)) {
            if (t.critCelsius.has_value() && *primary >= *t.critCelsius) {
                return true;
            }
            if (t.maxCelsius.has_value() && *primary >= *t.maxCelsius) {
                return true;
            }
        }
    }

    return *primary >= warnCelsius;
}

QString SensorSnapshot::trayTooltip(double warnCelsius) const
{
    QStringList parts;

    if (const auto cpu = primaryCpuCelsius()) {
        QString cpuPart = QStringLiteral("CPU %1°C").arg(*cpu, 0, 'f', 0);
        if (cpuIsHot(warnCelsius)) {
            cpuPart += QStringLiteral(" !");
        }
        parts << cpuPart;
    } else {
        parts << QStringLiteral("CPU N/A");
    }

    if (fans.isEmpty()) {
        parts << QStringLiteral("Fan N/A");
    } else {
        for (const FanReading &f : fans) {
            const QString name = f.label.isEmpty() ? QStringLiteral("Fan") : f.label;
            parts << QStringLiteral("%1 %2 RPM").arg(name).arg(f.rpm);
        }
    }

    return parts.join(QStringLiteral(" | "));
}
