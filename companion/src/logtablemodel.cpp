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

namespace {

QString formatDouble(double value)
{
  return QString::number(value, 'f', 3);
}

}

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

  if (!m_records)
    return QVariant();

  const int row = index.row();
  const int column = index.column();
  if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
    return QVariant();

  if (column < 0 || column >= m_columns)
    return QVariant();

  const LogRecord &rec = m_records->at(row);

  switch (m_columnIds.value(column, LogColumnId::Unknown)) {
    case LogColumnId::EpochMs:
      return rec.epochMs;
    case LogColumnId::Date:
      return rec.date.toString("yyyy-MM-dd");
    case LogColumnId::Time:
      return rec.time.toString("hh:mm:ss.zzz");
    case LogColumnId::Rss1:
      return rec.rss1;
    case LogColumnId::RQly:
      return rec.rqly;
    case LogColumnId::RSNR:
      return rec.rsnr;
    case LogColumnId::RxBt:
      return formatDouble(rec.rxBt);
    case LogColumnId::Curr:
      return formatDouble(rec.curr);
    case LogColumnId::Capa:
      return rec.capa;
    case LogColumnId::Alt:
      return formatDouble(rec.alt);
    case LogColumnId::VSpd:
      return formatDouble(rec.vspd);
    case LogColumnId::GPS:
      return rec.gps;
    case LogColumnId::GAlt:
      return formatDouble(rec.gAlt);
    case LogColumnId::GSpd:
      return formatDouble(rec.gSpd);
    case LogColumnId::Hdg:
      return rec.hdg;
    case LogColumnId::Sats:
      return rec.sats;
    case LogColumnId::FM:
      return rec.fm;
    case LogColumnId::Rud:
      return rec.rud;
    case LogColumnId::Ele:
      return rec.ele;
    case LogColumnId::Thr:
      return rec.thr;
    case LogColumnId::Ail:
      return rec.ail;
    case LogColumnId::S1:
      return rec.s1;
    case LogColumnId::S2:
      return rec.s2;
    case LogColumnId::SA:
      return rec.sa;
    case LogColumnId::SB:
      return rec.sb;
    case LogColumnId::SC:
      return rec.sc;
    case LogColumnId::SD:
      return rec.sd;
    case LogColumnId::LSW:
      return QString("0x%1").arg(rec.lsw, 16, 16, QChar('0')).toUpper();
    case LogColumnId::CH1:
      return rec.ch1;
    case LogColumnId::CH2:
      return rec.ch2;
    case LogColumnId::CH3:
      return rec.ch3;
    case LogColumnId::CH4:
      return rec.ch4;
    case LogColumnId::CH5:
      return rec.ch5;
    case LogColumnId::CH6:
      return rec.ch6;
    case LogColumnId::CH7:
      return rec.ch7;
    case LogColumnId::CH8:
      return rec.ch8;
    case LogColumnId::TxBat:
      return formatDouble(rec.txBat);
    default:
      return QVariant();
  }
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    if (section >= 0 && section < m_headers.size())
      return m_headers.at(section);
    return QVariant();
  }

  return section + 1;
}

void LogTableModel::setLog(const QVector<LogRecord> * records, const QStringList & headers, const QVector<LogColumnId> & columnIds)
{
  beginResetModel();

  m_records = nullptr;
  m_headers.clear();
  m_columnIds.clear();
  m_rows = 0;
  m_columns = 0;

  m_records = records;
  m_headers = headers;
  m_columnIds = columnIds;
  m_rows = m_records ? m_records->size() : 0;
  m_columns = m_headers.size();

  endResetModel();
}

void LogTableModel::clear()
{
  beginResetModel();
  m_records = nullptr;
  m_headers.clear();
  m_columnIds.clear();
  m_rows = 0;
  m_columns = 0;
  endResetModel();
}

QString LogTableModel::textAt(int row, int column) const
{
  if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
    return QString();

  if (!m_records)
    return QString();

  const QVariant value = data(index(row, column), Qt::DisplayRole);
  return value.toString();
}
