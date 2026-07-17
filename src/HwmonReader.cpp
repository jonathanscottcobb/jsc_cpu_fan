#include "HwmonReader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <iostream>

HwmonReader::HwmonReader(QString hwmonRoot, QString thermalRoot)
    : m_hwmonRoot(std::move(hwmonRoot))
    , m_thermalRoot(std::move(thermalRoot))
{
}

QString HwmonReader::readFileTrimmed(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

bool HwmonReader::readLong(const QString &path, qint64 *out)
{
    const QString text = readFileTrimmed(path);
    if (text.isEmpty()) {
        return false;
    }
    bool ok = false;
    const qint64 value = text.toLongLong(&ok);
    if (!ok) {
        return false;
    }
    *out = value;
    return true;
}

QVector<int> HwmonReader::channelIndices(const QString &chipDir, const QString &prefix)
{
    QVector<int> indices;
    const QDir dir(chipDir);
    const QStringList entries = dir.entryList(QStringList{prefix + QStringLiteral("*_input")},
                                              QDir::Files);

    const QRegularExpression re(
        QStringLiteral("^%1(\\d+)_input$").arg(QRegularExpression::escape(prefix)));

    for (const QString &entry : entries) {
        const QRegularExpressionMatch match = re.match(entry);
        if (match.hasMatch()) {
            indices.append(match.captured(1).toInt());
        }
    }

    std::sort(indices.begin(), indices.end());
    return indices;
}

QString HwmonReader::canonicalOrAbsolute(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}

QStringList HwmonReader::attributeRootsForChip(const QString &chipDir)
{
    QStringList candidates;

    // Modern drivers usually place attributes in the hwmon class directory.
    candidates.append(canonicalOrAbsolute(chipDir));

    // Older and multifunction drivers may place attributes on the physical
    // device reached through hwmonN/device. applesmc uses this layout.
    const QFileInfo deviceInfo(chipDir + QStringLiteral("/device"));
    if (deviceInfo.exists()) {
        candidates.append(canonicalOrAbsolute(deviceInfo.filePath()));
    }

    // Some kernels omit the device symlink. A canonical path ending in
    // .../hwmon/hwmonN still tells us where the physical parent is.
    const QDir hwmonDir(candidates.first());
    const QDir hwmonContainer = QFileInfo(hwmonDir.absolutePath()).dir();
    if (hwmonContainer.dirName() == QStringLiteral("hwmon")) {
        candidates.append(canonicalOrAbsolute(
            QFileInfo(hwmonContainer.absolutePath()).dir().absolutePath()));
    }

    QStringList roots;
    QSet<QString> seen;
    for (const QString &candidate : candidates) {
        if (seen.contains(candidate)) {
            continue;
        }
        if (!channelIndices(candidate, QStringLiteral("temp")).isEmpty()
            || !channelIndices(candidate, QStringLiteral("fan")).isEmpty()) {
            roots.append(candidate);
            seen.insert(candidate);
        }
    }
    return roots;
}

QString HwmonReader::chipName(const QString &chipDir, const QStringList &attributeRoots)
{
    QString name = readFileTrimmed(chipDir + QStringLiteral("/name"));
    for (const QString &root : attributeRoots) {
        if (!name.isEmpty()) {
            break;
        }
        name = readFileTrimmed(root + QStringLiteral("/name"));
    }
    return name.isEmpty() ? QFileInfo(chipDir).fileName() : name;
}

void HwmonReader::readHwmonRoot(const QString &attributeRoot, const QString &chipLabel,
                                SensorSnapshot *snapshot, QSet<QString> *seenInputs)
{
    for (int idx : channelIndices(attributeRoot, QStringLiteral("temp"))) {
        const QString inputPath =
            attributeRoot + QStringLiteral("/temp%1_input").arg(idx);
        const QString source = canonicalOrAbsolute(inputPath);
        if (seenInputs->contains(source)) {
            continue;
        }

        qint64 milliC = 0;
        if (!readLong(inputPath, &milliC)) {
            continue;
        }

        TempReading reading;
        reading.source = source;
        reading.chip = chipLabel;
        reading.label =
            readFileTrimmed(attributeRoot + QStringLiteral("/temp%1_label").arg(idx));
        if (reading.label.isEmpty()) {
            reading.label = QStringLiteral("%1 temp%2").arg(chipLabel).arg(idx);
        }
        reading.celsius = milliC / 1000.0;

        qint64 threshold = 0;
        if (readLong(attributeRoot + QStringLiteral("/temp%1_max").arg(idx), &threshold)) {
            reading.maxCelsius = threshold / 1000.0;
        }
        if (readLong(attributeRoot + QStringLiteral("/temp%1_crit").arg(idx), &threshold)) {
            reading.critCelsius = threshold / 1000.0;
        }

        snapshot->temps.append(reading);
        seenInputs->insert(source);
    }

    for (int idx : channelIndices(attributeRoot, QStringLiteral("fan"))) {
        const QString inputPath =
            attributeRoot + QStringLiteral("/fan%1_input").arg(idx);
        const QString source = canonicalOrAbsolute(inputPath);
        if (seenInputs->contains(source)) {
            continue;
        }

        qint64 rpm = 0;
        if (!readLong(inputPath, &rpm)) {
            continue;
        }

        FanReading reading;
        reading.source = source;
        reading.chip = chipLabel;
        reading.label =
            readFileTrimmed(attributeRoot + QStringLiteral("/fan%1_label").arg(idx));
        if (reading.label.isEmpty()) {
            reading.label = QStringLiteral("%1 fan%2").arg(chipLabel).arg(idx);
        }
        reading.rpm = rpm;
        snapshot->fans.append(reading);
        seenInputs->insert(source);
    }
}

void HwmonReader::readThermalFallback(SensorSnapshot *snapshot) const
{
    if (!snapshot->temps.isEmpty()) {
        return;
    }

    const QDir thermalRoot(m_thermalRoot);
    const QStringList zones = thermalRoot.entryList(
        QStringList{QStringLiteral("thermal_zone*")},
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const QString &zone : zones) {
        const QString zonePath = thermalRoot.filePath(zone);
        qint64 milliC = 0;
        if (!readLong(zonePath + QStringLiteral("/temp"), &milliC)) {
            continue;
        }

        TempReading reading;
        reading.source = canonicalOrAbsolute(zonePath + QStringLiteral("/temp"));
        reading.chip = QStringLiteral("thermal");
        reading.label = readFileTrimmed(zonePath + QStringLiteral("/type"));
        if (reading.label.isEmpty()) {
            reading.label = zone;
        }
        reading.celsius = milliC / 1000.0;
        snapshot->temps.append(reading);
    }
}

SensorSnapshot HwmonReader::read() const
{
    SensorSnapshot snapshot;

    QDir root(m_hwmonRoot);
    if (!root.exists()) {
        readThermalFallback(&snapshot);
        return snapshot;
    }

    const QStringList chips = root.entryList(QStringList{QStringLiteral("hwmon*")},
                                             QDir::Dirs | QDir::NoDotAndDotDot,
                                             QDir::Name);

    QSet<QString> seenInputs;
    for (const QString &chipName : chips) {
        const QString chipDir = root.filePath(chipName);
        const QStringList attributeRoots = attributeRootsForChip(chipDir);
        const QString label = HwmonReader::chipName(chipDir, attributeRoots);
        for (const QString &attributeRoot : attributeRoots) {
            readHwmonRoot(attributeRoot, label, &snapshot, &seenInputs);
        }
    }

    readThermalFallback(&snapshot);
    return snapshot;
}

void HwmonReader::dumpToStdout() const
{
    const SensorSnapshot snapshot = read();

    std::cout << "=== JSC CPU hwmon dump ===\n";
    std::cout << "Primary CPU: ";
    if (const auto cpu = snapshot.primaryCpuCelsius()) {
        std::cout << *cpu << " C\n";
    } else {
        std::cout << "N/A\n";
    }
    std::cout << "Tooltip: " << snapshot.trayTooltip().toStdString() << "\n\n";

    std::cout << "Temperatures (" << snapshot.temps.size() << "):\n";
    for (const TempReading &t : snapshot.temps) {
        std::cout << "  [" << t.chip.toStdString() << "] " << t.label.toStdString()
                  << " = " << t.celsius << " C";
        if (t.maxCelsius) {
            std::cout << " (max " << *t.maxCelsius << ")";
        }
        if (t.critCelsius) {
            std::cout << " (crit " << *t.critCelsius << ")";
        }
        std::cout << "\n";
    }

    std::cout << "\nFans (" << snapshot.fans.size() << "):\n";
    for (const FanReading &f : snapshot.fans) {
        std::cout << "  [" << f.chip.toStdString() << "] " << f.label.toStdString()
                  << " = " << f.rpm << " RPM\n";
    }
}
