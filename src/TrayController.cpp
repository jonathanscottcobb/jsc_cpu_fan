#include "TrayController.h"

#include "DetailPopup.h"

#include <QAction>
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpression>
#include <QTimer>

namespace {

constexpr int kIconSize = 64; // painted large, scaled down by the shell

QString shortTempValue(double celsius)
{
    return QStringLiteral("%1°").arg(qRound(celsius));
}

QString shortRpmValue(qint64 rpm)
{
    if (rpm >= 1000) {
        return QStringLiteral("%1k").arg(rpm / 1000.0, 0, 'f', 1);
    }
    return QString::number(rpm);
}

} // namespace

TrayController::TrayController(QObject *parent)
    : QObject(parent)
{
    m_menu = new QMenu;
    QAction *showAction = m_menu->addAction(QStringLiteral("Show details"));
    QAction *refreshAction = m_menu->addAction(QStringLiteral("Refresh now"));
    m_menu->addSeparator();
    QAction *quitAction = m_menu->addAction(QStringLiteral("Quit"));

    connect(showAction, &QAction::triggered, this, [this] { showDetails(); });
    connect(refreshAction, &QAction::triggered, this, [this] { refresh(); });
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_popup = new DetailPopup;

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, [this] { refresh(); });

    refresh();
    m_timer->start();
}

TrayController::~TrayController()
{
    delete m_popup;
    delete m_menu;
}

QString TrayController::abbreviateFanLabel(const QString &label, int index)
{
    const QString lower = label.toLower();
    if (lower.contains(QStringLiteral("exhaust"))) {
        return QStringLiteral("Ex");
    }
    if (lower.contains(QStringLiteral("intake"))) {
        return QStringLiteral("In");
    }
    if (lower.contains(QStringLiteral("cpu"))) {
        return QStringLiteral("CPU");
    }
    if (lower.contains(QStringLiteral("gpu"))) {
        return QStringLiteral("GPU");
    }
    if (lower.contains(QStringLiteral("chassis")) || lower.contains(QStringLiteral("case"))) {
        return QStringLiteral("Ch");
    }
    // Generic fallback: F1, F2, ...
    return QStringLiteral("F%1").arg(index + 1);
}

QVector<TrayController::SensorIconEntry> TrayController::buildEntries() const
{
    QVector<SensorIconEntry> entries;

    // Covers common labels such as "Core 0", "CPU Core 3",
    // "core_7", and "Core12" without depending on a driver name.
    static const QRegularExpression coreRe(
        QStringLiteral("^(?:CPU[\\s_-]*)?Core[\\s_-]*(\\d+)$"),
        QRegularExpression::CaseInsensitiveOption);

    bool foundCore = false;
    for (const TempReading &t : m_lastSnapshot.temps) {
        const QRegularExpressionMatch match = coreRe.match(t.label);
        if (!match.hasMatch()) {
            continue;
        }
        foundCore = true;

        const bool hot = (t.critCelsius && t.celsius >= *t.critCelsius)
            || (t.maxCelsius && t.celsius >= *t.maxCelsius)
            || t.celsius >= 85.0;

        SensorIconEntry e;
        e.key = QStringLiteral("temp:%1").arg(t.source);
        e.shortLabel = QStringLiteral("C%1").arg(match.captured(1));
        e.valueText = shortTempValue(t.celsius);
        e.tooltip = QStringLiteral("%1 %2: %3°C").arg(t.chip, t.label).arg(t.celsius, 0, 'f', 1);
        e.hot = hot;
        entries.append(e);
    }

    // No per-core channels on this chip: fall back to one icon for the CPU.
    if (!foundCore) {
        if (const auto cpu = m_lastSnapshot.primaryCpuCelsius()) {
            SensorIconEntry e;
            e.key = QStringLiteral("temp:primary");
            e.shortLabel = QStringLiteral("CPU");
            e.valueText = shortTempValue(*cpu);
            e.tooltip = QStringLiteral("CPU: %1°C").arg(*cpu, 0, 'f', 1);
            e.hot = m_lastSnapshot.cpuIsHot();
            entries.append(e);
        }
    }

    int fanIndex = 0;
    for (const FanReading &f : m_lastSnapshot.fans) {
        SensorIconEntry e;
        e.key = QStringLiteral("fan:%1").arg(f.source);
        e.shortLabel = abbreviateFanLabel(f.label, fanIndex);
        e.valueText = shortRpmValue(f.rpm);
        e.tooltip = QStringLiteral("%1 %2: %3 RPM").arg(f.chip, f.label).arg(f.rpm);
        entries.append(e);
        ++fanIndex;
    }

    return entries;
}

QIcon TrayController::paintNumberIcon(const QString &label, const QString &value, bool hot)
{
    QPixmap pixmap(kIconSize, kIconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const QRect labelRect(0, 0, kIconSize, kIconSize * 2 / 5);
    const QRect valueRect(0, kIconSize * 2 / 5, kIconSize, kIconSize * 3 / 5);

    QFont labelFont = QApplication::font();
    labelFont.setPixelSize(kIconSize * 2 / 5 - 2);
    labelFont.setBold(true);

    QFont valueFont = labelFont;
    valueFont.setPixelSize(kIconSize * 3 / 5 - 6);

    const QColor valueColor = hot ? QColor(QStringLiteral("#e74c3c")) : QColor(Qt::white);
    const QColor labelColor(QStringLiteral("#bbbbbb"));
    const QColor shadow(0, 0, 0, 180);

    // 1px shadow keeps the text readable on both light and dark tray themes.
    painter.setFont(labelFont);
    painter.setPen(shadow);
    painter.drawText(labelRect.translated(1, 1), Qt::AlignCenter, label);
    painter.setPen(labelColor);
    painter.drawText(labelRect, Qt::AlignCenter, label);

    painter.setFont(valueFont);
    painter.setPen(shadow);
    painter.drawText(valueRect.translated(1, 1), Qt::AlignCenter, value);
    painter.setPen(valueColor);
    painter.drawText(valueRect, Qt::AlignCenter, value);

    painter.end();
    return QIcon(pixmap);
}

void TrayController::syncIconCount(const QStringList &keys)
{
    if (keys == m_iconKeys) {
        return;
    }

    qDeleteAll(m_icons);
    m_icons.clear();

    for (int i = 0; i < keys.size(); ++i) {
        auto *icon = new QSystemTrayIcon(this);
        icon->setContextMenu(m_menu);
        connect(icon, &QSystemTrayIcon::activated, this,
                [this](QSystemTrayIcon::ActivationReason reason) {
                    if (reason == QSystemTrayIcon::Trigger
                        || reason == QSystemTrayIcon::DoubleClick) {
                        showDetails();
                    }
                });
        m_icons.append(icon);
    }

    m_iconKeys = keys;
}

void TrayController::refresh()
{
    m_lastSnapshot = m_reader.read();

    const QVector<SensorIconEntry> entries = buildEntries();

    QStringList keys;
    keys.reserve(entries.size());
    for (const SensorIconEntry &e : entries) {
        keys.append(e.key);
    }
    syncIconCount(keys);

    for (int i = 0; i < entries.size(); ++i) {
        const SensorIconEntry &e = entries.at(i);
        QSystemTrayIcon *icon = m_icons.at(i);
        icon->setIcon(paintNumberIcon(e.shortLabel, e.valueText, e.hot));
        icon->setToolTip(e.tooltip);
        if (!icon->isVisible()) {
            icon->show();
        }
    }

    if (m_popup && m_popup->isVisible()) {
        m_popup->updateSnapshot(m_lastSnapshot);
    }
}

void TrayController::showDetails()
{
    if (!m_popup) {
        return;
    }
    m_popup->updateSnapshot(m_lastSnapshot);
    m_popup->show();
    m_popup->raise();
    m_popup->activateWindow();
}
