#pragma once

#include "HwmonReader.h"
#include "SensorModel.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QVector>

class DetailPopup;
class QMenu;
class QTimer;

class TrayController : public QObject
{
public:
    explicit TrayController(QObject *parent = nullptr);
    ~TrayController() override;

    void refresh();
    void showDetails();

private:
    // One tray entry per sensor reading (a CPU core temp or a fan).
    struct SensorIconEntry {
        QString key;        // stable identity, e.g. "temp:coretemp:Core 0"
        QString shortLabel; // rendered top row, e.g. "C0", "Ex"
        QString valueText;  // rendered bottom row, e.g. "72°", "1.8k"
        QString tooltip;
        bool hot = false;
    };

    HwmonReader m_reader;
    SensorSnapshot m_lastSnapshot;
    QVector<QSystemTrayIcon *> m_icons;
    QStringList m_iconKeys;
    QMenu *m_menu = nullptr;
    DetailPopup *m_popup = nullptr;
    QTimer *m_timer = nullptr;

    QVector<SensorIconEntry> buildEntries() const;
    void syncIconCount(const QStringList &keys);
    static QIcon paintNumberIcon(const QString &label, const QString &value, bool hot);
    static QString abbreviateFanLabel(const QString &label, int index);
};
