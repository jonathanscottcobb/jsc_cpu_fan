#include "DetailPopup.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QVBoxLayout>

DetailPopup::DetailPopup(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("JSC CPU — Sensors"));
    setWindowFlag(Qt::Tool);
    resize(420, 360);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Type"), QStringLiteral("Label"), QStringLiteral("Value")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_table);
}

void DetailPopup::updateSnapshot(const SensorSnapshot &snapshot)
{
    const int rows = snapshot.temps.size() + snapshot.fans.size();
    m_table->setRowCount(rows);

    int row = 0;
    for (const TempReading &t : snapshot.temps) {
        auto *typeItem = new QTableWidgetItem(QStringLiteral("Temp"));
        auto *labelItem = new QTableWidgetItem(
            QStringLiteral("%1 / %2").arg(t.chip, t.label));
        auto *valueItem = new QTableWidgetItem(
            QStringLiteral("%1 °C").arg(t.celsius, 0, 'f', 1));

        if (snapshot.cpuIsHot()) {
            const auto primary = snapshot.primaryCpuCelsius();
            if (primary && qFuzzyCompare(t.celsius + 1.0, *primary + 1.0)) {
                valueItem->setForeground(QBrush(QColor(QStringLiteral("#c0392b"))));
            }
        }

        m_table->setItem(row, 0, typeItem);
        m_table->setItem(row, 1, labelItem);
        m_table->setItem(row, 2, valueItem);
        ++row;
    }

    for (const FanReading &f : snapshot.fans) {
        m_table->setItem(row, 0, new QTableWidgetItem(QStringLiteral("Fan")));
        m_table->setItem(row, 1,
                         new QTableWidgetItem(QStringLiteral("%1 / %2").arg(f.chip, f.label)));
        m_table->setItem(row, 2,
                         new QTableWidgetItem(QStringLiteral("%1 RPM").arg(f.rpm)));
        ++row;
    }
}
