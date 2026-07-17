#pragma once

#include "SensorModel.h"

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

class HwmonReader
{
public:
    static constexpr const char *kDefaultHwmonPath = "/sys/class/hwmon";
    static constexpr const char *kDefaultThermalPath = "/sys/class/thermal";

    explicit HwmonReader(
        QString hwmonRoot = QString::fromLatin1(kDefaultHwmonPath),
        QString thermalRoot = QString::fromLatin1(kDefaultThermalPath));

    SensorSnapshot read() const;
    void dumpToStdout() const;

private:
    QString m_hwmonRoot;
    QString m_thermalRoot;

    static QString readFileTrimmed(const QString &path);
    static bool readLong(const QString &path, qint64 *out);
    static QVector<int> channelIndices(const QString &chipDir, const QString &prefix);
    static QStringList attributeRootsForChip(const QString &chipDir);
    static QString canonicalOrAbsolute(const QString &path);
    static QString chipName(const QString &chipDir, const QStringList &attributeRoots);
    static void readHwmonRoot(const QString &attributeRoot, const QString &chipLabel,
                              SensorSnapshot *snapshot, QSet<QString> *seenInputs);
    void readThermalFallback(SensorSnapshot *snapshot) const;
};
