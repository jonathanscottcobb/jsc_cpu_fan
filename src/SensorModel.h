#pragma once

#include <QString>
#include <QVector>
#include <optional>

struct TempReading {
    QString source;
    QString chip;
    QString label;
    double celsius = 0.0;
    std::optional<double> maxCelsius;
    std::optional<double> critCelsius;
};

struct FanReading {
    QString source;
    QString chip;
    QString label;
    qint64 rpm = 0;
};

struct SensorSnapshot {
    QVector<TempReading> temps;
    QVector<FanReading> fans;

    std::optional<double> primaryCpuCelsius() const;
    QString trayTooltip(double warnCelsius = 85.0) const;
    bool cpuIsHot(double warnCelsius = 85.0) const;
};
