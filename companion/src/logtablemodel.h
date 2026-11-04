/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#pragma once

#include <QAbstractTableModel>
#include "logrecord.h"
#include <QStringList>
#include <vector>

// Read-only model for the telemetry log table. Data is stored in a flat
// std::vector: row-major, m_columnCount entries per row.
class LogTableModel : public QAbstractTableModel
{
public:
  explicit LogTableModel(QObject * parent = nullptr);

  int rowCount(const QModelIndex & parent = QModelIndex()) const override;
  int columnCount(const QModelIndex & parent = QModelIndex()) const override;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  void setLog(const QVector<LogRecord> & records, const QStringList & headers, const QVector<LogColumnId> & columnIds);
  void clear();

  QString textAt(int row, int column) const;

private:
  QVector<LogRecord> m_records;
  QStringList m_headers;
  QVector<LogColumnId> m_columnIds;
  int m_rows = 0;
  int m_columns = 0;
};
