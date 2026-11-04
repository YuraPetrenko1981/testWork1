#include "logparser.h"
#include <QDateTime>
#include <QStringList>

namespace {

bool parseHexU64(const QString & text, quint64 & value)
{
  QString trimmed = text.trimmed();
  if (trimmed.startsWith("0x") || trimmed.startsWith("0X"))
    trimmed = trimmed.mid(2);
  bool ok = false;
  value = trimmed.toULongLong(&ok, 16);
  return ok;
}

bool parseInt(const QString & text, int & value)
{
  bool ok = false;
  value = text.toInt(&ok);
  return ok;
}

bool parseDouble(const QString & text, double & value)
{
  bool ok = false;
  value = text.toDouble(&ok);
  return ok;
}

}

LogColumnId logColumnIdFromHeader(const QString & header)
{
  if (header == "Date") return LogColumnId::Date;
  if (header == "Time") return LogColumnId::Time;
  if (header == "1RSS(dB)") return LogColumnId::Rss1;
  if (header == "RQly(%)") return LogColumnId::RQly;
  if (header == "RSNR(dB)") return LogColumnId::RSNR;
  if (header == "RxBt(V)") return LogColumnId::RxBt;
  if (header == "Curr(A)") return LogColumnId::Curr;
  if (header == "Capa(mAh)") return LogColumnId::Capa;
  if (header == "Alt(m)") return LogColumnId::Alt;
  if (header == "VSpd(m/s)") return LogColumnId::VSpd;
  if (header == "GPS") return LogColumnId::GPS;
  if (header == "GAlt" || header == "GAlt(m)") return LogColumnId::GAlt;
  if (header == "GSpd" || header == "GSpd(kmh)") return LogColumnId::GSpd;
  if (header == "Hdg(@)") return LogColumnId::Hdg;
  if (header == "Sats") return LogColumnId::Sats;
  if (header == "FM") return LogColumnId::FM;
  if (header == "Rud") return LogColumnId::Rud;
  if (header == "Ele") return LogColumnId::Ele;
  if (header == "Thr") return LogColumnId::Thr;
  if (header == "Ail") return LogColumnId::Ail;
  if (header == "S1") return LogColumnId::S1;
  if (header == "S2") return LogColumnId::S2;
  if (header == "SA") return LogColumnId::SA;
  if (header == "SB") return LogColumnId::SB;
  if (header == "SC") return LogColumnId::SC;
  if (header == "SD") return LogColumnId::SD;
  if (header == "LSW") return LogColumnId::LSW;
  if (header == "CH1(us)") return LogColumnId::CH1;
  if (header == "CH2(us)") return LogColumnId::CH2;
  if (header == "CH3(us)") return LogColumnId::CH3;
  if (header == "CH4(us)") return LogColumnId::CH4;
  if (header == "CH5(us)") return LogColumnId::CH5;
  if (header == "CH6(us)") return LogColumnId::CH6;
  if (header == "CH7(us)") return LogColumnId::CH7;
  if (header == "CH8(us)") return LogColumnId::CH8;
  if (header == "TxBat(V)") return LogColumnId::TxBat;
  return LogColumnId::Unknown;
}

LogParseResult parseLogCsvText(const QString & text)
{
  LogParseResult result;
  result.errors = 0;
  result.lines = -1;

  QStringList lines = text.split('\n');
  if (lines.isEmpty()) {
    result.lines = 0;
    return result;
  }

  QString firstLineText = lines.first().trimmed();
  if (!firstLineText.startsWith("Date,Time")) {
    result.errors++;
    result.lines = lines.size();
    return result;
  }

  int numfields = -1;
  for (const QString & rawLine : lines) {
    QString line = rawLine.trimmed();
    if (line.isEmpty())
      continue;
    QStringList columns = line.split(',');

    if (numfields == -1) {
      numfields = columns.count();
    }

    if (columns.count() == numfields) {
      if (result.lines == -1) {
        result.headers.clear();
        result.headers.append("MSecsSinceEpoch");
        result.headers.append(columns);
        result.columnIds.clear();
        result.columnIds.reserve(result.headers.size());
        result.columnIds.append(LogColumnId::EpochMs);
        for (const QString & header : columns) {
          result.columnIds.append(logColumnIdFromHeader(header));
        }
      } else {
        LogRecord record;
        bool ok = true;

        record.date = QDate::fromString(columns.at(0), "yyyy-MM-dd");
        record.time = QTime::fromString(columns.at(1), "hh:mm:ss.zzz");
        if (!record.date.isValid() || !record.time.isValid())
          ok = false;

        if (ok) {
          record.epochMs = QDateTime(record.date, record.time).toMSecsSinceEpoch();
        }

        for (int i = 2; i < columns.size(); ++i) {
          const QString & value = columns.at(i);
          switch (result.columnIds.at(i + 1)) {
            case LogColumnId::Rss1:
              ok = parseInt(value, record.rss1) && ok;
              break;
            case LogColumnId::RQly:
              ok = parseInt(value, record.rqly) && ok;
              break;
            case LogColumnId::RSNR:
              ok = parseInt(value, record.rsnr) && ok;
              break;
            case LogColumnId::RxBt:
              ok = parseDouble(value, record.rxBt) && ok;
              break;
            case LogColumnId::Curr:
              ok = parseDouble(value, record.curr) && ok;
              break;
            case LogColumnId::Capa:
              ok = parseInt(value, record.capa) && ok;
              break;
            case LogColumnId::Alt:
              ok = parseDouble(value, record.alt) && ok;
              break;
            case LogColumnId::VSpd:
              ok = parseDouble(value, record.vspd) && ok;
              break;
            case LogColumnId::GPS:
              record.gps = value;
              break;
            case LogColumnId::GAlt:
              ok = parseDouble(value, record.gAlt) && ok;
              break;
            case LogColumnId::GSpd:
              ok = parseDouble(value, record.gSpd) && ok;
              break;
            case LogColumnId::Hdg:
              ok = parseInt(value, record.hdg) && ok;
              break;
            case LogColumnId::Sats:
              ok = parseInt(value, record.sats) && ok;
              break;
            case LogColumnId::FM:
              record.fm = value;
              break;
            case LogColumnId::Rud:
              ok = parseInt(value, record.rud) && ok;
              break;
            case LogColumnId::Ele:
              ok = parseInt(value, record.ele) && ok;
              break;
            case LogColumnId::Thr:
              ok = parseInt(value, record.thr) && ok;
              break;
            case LogColumnId::Ail:
              ok = parseInt(value, record.ail) && ok;
              break;
            case LogColumnId::S1:
              ok = parseInt(value, record.s1) && ok;
              break;
            case LogColumnId::S2:
              ok = parseInt(value, record.s2) && ok;
              break;
            case LogColumnId::SA:
              ok = parseInt(value, record.sa) && ok;
              break;
            case LogColumnId::SB:
              ok = parseInt(value, record.sb) && ok;
              break;
            case LogColumnId::SC:
              ok = parseInt(value, record.sc) && ok;
              break;
            case LogColumnId::SD:
              ok = parseInt(value, record.sd) && ok;
              break;
            case LogColumnId::LSW:
              ok = parseHexU64(value, record.lsw) && ok;
              break;
            case LogColumnId::CH1:
              ok = parseInt(value, record.ch1) && ok;
              break;
            case LogColumnId::CH2:
              ok = parseInt(value, record.ch2) && ok;
              break;
            case LogColumnId::CH3:
              ok = parseInt(value, record.ch3) && ok;
              break;
            case LogColumnId::CH4:
              ok = parseInt(value, record.ch4) && ok;
              break;
            case LogColumnId::CH5:
              ok = parseInt(value, record.ch5) && ok;
              break;
            case LogColumnId::CH6:
              ok = parseInt(value, record.ch6) && ok;
              break;
            case LogColumnId::CH7:
              ok = parseInt(value, record.ch7) && ok;
              break;
            case LogColumnId::CH8:
              ok = parseInt(value, record.ch8) && ok;
              break;
            case LogColumnId::TxBat:
              ok = parseDouble(value, record.txBat) && ok;
              break;
            case LogColumnId::Date:
            case LogColumnId::Time:
            case LogColumnId::EpochMs:
            case LogColumnId::Unknown:
            default:
              break;
          }
        }

        if (ok) {
          result.records.append(record);
        } else {
          result.errors++;
        }
      }
    } else {
      result.errors++;
    }
    result.lines++;
  }

  return result;
}

LogParseResult parseLogCsvDevice(QIODevice & device, qint64 fileSize,
                                 const std::function<void(int)> & progress,
                                 const std::atomic<bool> * cancelRequested)
{
  LogParseResult result;
  result.errors = 0;
  result.lines = -1;

  if (!device.isOpen())
    return result;

  QByteArray firstLine = device.readLine();
  qint64 bytesRead = firstLine.size();
  QString firstLineText = QString::fromLatin1(firstLine).trimmed();
  if (!firstLineText.startsWith("Date,Time")) {
    result.errors++;
    result.lines = 1;
    return result;
  }

  device.seek(0);
  bytesRead = 0;
  int lastPercent = -1;
  int numfields = -1;

  while (!device.atEnd()) {
    if (cancelRequested && cancelRequested->load())
      return result;

    QByteArray lineBytes = device.readLine();
    bytesRead += lineBytes.size();
    QString line = QString::fromLatin1(lineBytes).trimmed();
    if (line.isEmpty())
      continue;
    QStringList columns = line.split(',');

    if (numfields == -1)
      numfields = columns.count();

    if (columns.count() == numfields) {
      if (result.lines == -1) {
        result.headers.clear();
        result.headers.append("MSecsSinceEpoch");
        result.headers.append(columns);
        result.columnIds.clear();
        result.columnIds.reserve(result.headers.size());
        result.columnIds.append(LogColumnId::EpochMs);
        for (const QString & header : columns) {
          result.columnIds.append(logColumnIdFromHeader(header));
        }
      } else {
        LogRecord record;
        bool ok = true;

        record.date = QDate::fromString(columns.at(0), "yyyy-MM-dd");
        record.time = QTime::fromString(columns.at(1), "hh:mm:ss.zzz");
        if (!record.date.isValid() || !record.time.isValid())
          ok = false;

        if (ok) {
          record.epochMs = QDateTime(record.date, record.time).toMSecsSinceEpoch();
        }

        for (int i = 2; i < columns.size(); ++i) {
          const QString & value = columns.at(i);
          switch (result.columnIds.at(i + 1)) {
            case LogColumnId::Rss1:
              ok = parseInt(value, record.rss1) && ok;
              break;
            case LogColumnId::RQly:
              ok = parseInt(value, record.rqly) && ok;
              break;
            case LogColumnId::RSNR:
              ok = parseInt(value, record.rsnr) && ok;
              break;
            case LogColumnId::RxBt:
              ok = parseDouble(value, record.rxBt) && ok;
              break;
            case LogColumnId::Curr:
              ok = parseDouble(value, record.curr) && ok;
              break;
            case LogColumnId::Capa:
              ok = parseInt(value, record.capa) && ok;
              break;
            case LogColumnId::Alt:
              ok = parseDouble(value, record.alt) && ok;
              break;
            case LogColumnId::VSpd:
              ok = parseDouble(value, record.vspd) && ok;
              break;
            case LogColumnId::GPS:
              record.gps = value;
              break;
            case LogColumnId::GAlt:
              ok = parseDouble(value, record.gAlt) && ok;
              break;
            case LogColumnId::GSpd:
              ok = parseDouble(value, record.gSpd) && ok;
              break;
            case LogColumnId::Hdg:
              ok = parseInt(value, record.hdg) && ok;
              break;
            case LogColumnId::Sats:
              ok = parseInt(value, record.sats) && ok;
              break;
            case LogColumnId::FM:
              record.fm = value;
              break;
            case LogColumnId::Rud:
              ok = parseInt(value, record.rud) && ok;
              break;
            case LogColumnId::Ele:
              ok = parseInt(value, record.ele) && ok;
              break;
            case LogColumnId::Thr:
              ok = parseInt(value, record.thr) && ok;
              break;
            case LogColumnId::Ail:
              ok = parseInt(value, record.ail) && ok;
              break;
            case LogColumnId::S1:
              ok = parseInt(value, record.s1) && ok;
              break;
            case LogColumnId::S2:
              ok = parseInt(value, record.s2) && ok;
              break;
            case LogColumnId::SA:
              ok = parseInt(value, record.sa) && ok;
              break;
            case LogColumnId::SB:
              ok = parseInt(value, record.sb) && ok;
              break;
            case LogColumnId::SC:
              ok = parseInt(value, record.sc) && ok;
              break;
            case LogColumnId::SD:
              ok = parseInt(value, record.sd) && ok;
              break;
            case LogColumnId::LSW:
              ok = parseHexU64(value, record.lsw) && ok;
              break;
            case LogColumnId::CH1:
              ok = parseInt(value, record.ch1) && ok;
              break;
            case LogColumnId::CH2:
              ok = parseInt(value, record.ch2) && ok;
              break;
            case LogColumnId::CH3:
              ok = parseInt(value, record.ch3) && ok;
              break;
            case LogColumnId::CH4:
              ok = parseInt(value, record.ch4) && ok;
              break;
            case LogColumnId::CH5:
              ok = parseInt(value, record.ch5) && ok;
              break;
            case LogColumnId::CH6:
              ok = parseInt(value, record.ch6) && ok;
              break;
            case LogColumnId::CH7:
              ok = parseInt(value, record.ch7) && ok;
              break;
            case LogColumnId::CH8:
              ok = parseInt(value, record.ch8) && ok;
              break;
            case LogColumnId::TxBat:
              ok = parseDouble(value, record.txBat) && ok;
              break;
            case LogColumnId::Date:
            case LogColumnId::Time:
            case LogColumnId::EpochMs:
            case LogColumnId::Unknown:
            default:
              break;
          }
        }

        if (ok) {
          result.records.append(record);
        } else {
          result.errors++;
        }
      }
    } else {
      result.errors++;
    }
    result.lines++;

    if (progress && fileSize > 0) {
      int percent = static_cast<int>((bytesRead * 100) / fileSize);
      if (percent != lastPercent) {
        lastPercent = percent;
        progress(percent);
      }
    }
  }

  return result;
}
