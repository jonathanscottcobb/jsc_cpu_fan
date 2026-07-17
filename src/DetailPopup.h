#pragma once

#include "SensorModel.h"

#include <QDialog>
#include <QTableWidget>

class DetailPopup : public QDialog
{
    Q_OBJECT

public:
    explicit DetailPopup(QWidget *parent = nullptr);

    void updateSnapshot(const SensorSnapshot &snapshot);

private:
    QTableWidget *m_table = nullptr;
};
