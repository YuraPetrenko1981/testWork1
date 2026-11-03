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

#include "logtablemodel.h"

LogTableModel::LogTableModel(QObject * parent) :
  QAbstractTableModel(parent)
{
}

int LogTableModel::rowCount(const QModelIndex & parent) const
{
  return parent.isValid() ? 0 : m_rows;
}

int LogTableModel::columnCount(const QModelIndex & parent) const
{
  return parent.isValid() ? 0 : m_columns;
}

QVariant LogTableModel::data(const QModelIndex & index, int role) const
{
  if (!index.isValid() || role != Qt::DisplayRole)
    return QVariant();

  const int row = index.row();
  const int column = index.column();
  if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
    return QVariant();

  return m_cells[static_cast<size_t>(row) * static_cast<size_t>(m_columns) +
                 static_cast<size_t>(column)];
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    if (section >= 0 && section < static_cast<int>(m_headers.size()))
      return m_headers[static_cast<size_t>(section)];
    return QVariant();
  }

  return section + 1;
}

void LogTableModel::setLog(const QList<QStringList> & log)
{
  beginResetModel();

  m_cells.clear();
  m_headers.clear();
  m_rows = 0;
  m_columns = 0;

  if (!log.isEmpty()) {
    const QStringList & header = log.first();
    m_columns = header.size();
    m_rows = log.size() - 1;
    m_headers.assign(header.begin(), header.end());

    m_cells.reserve(static_cast<size_t>(m_rows) * static_cast<size_t>(m_columns));

    for (int row = 1; row < log.size(); row++) {
      const QStringList & values = log.at(row);
      for (int column = 0; column < m_columns; column++) {
        m_cells.push_back(column < values.size() ? values.at(column) : QString());
      }
    }
  }

  endResetModel();
}

void LogTableModel::clear()
{
  beginResetModel();
  m_cells.clear();
  m_headers.clear();
  m_rows = 0;
  m_columns = 0;
  endResetModel();
}

QString LogTableModel::textAt(int row, int column) const
{
  if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
    return QString();

  return m_cells[static_cast<size_t>(row) * static_cast<size_t>(m_columns) +
                 static_cast<size_t>(column)];
}
