#pragma once

#include <QtCore>

enum class LogColumnId {
  EpochMs,
  Date,
  Time,
  Rss1,
  RQly,
  RSNR,
  RxBt,
  Curr,
  Capa,
  Alt,
  VSpd,
  GPS,
  GAlt,
  GSpd,
  Hdg,
  Sats,
  FM,
  Rud,
  Ele,
  Thr,
  Ail,
  S1,
  S2,
  SA,
  SB,
  SC,
  SD,
  LSW,
  CH1,
  CH2,
  CH3,
  CH4,
  CH5,
  CH6,
  CH7,
  CH8,
  TxBat,
  Unknown
};

struct LogRecord {
  qint64 epochMs = 0;
  QDate date;
  QTime time;
  int rss1 = 0;
  int rqly = 0;
  int rsnr = 0;
  double rxBt = 0.0;
  double curr = 0.0;
  int capa = 0;
  double alt = 0.0;
  double vspd = 0.0;
  QString gps;
  double gAlt = 0.0;
  double gSpd = 0.0;
  int hdg = 0;
  int sats = 0;
  QString fm;
  int rud = 0;
  int ele = 0;
  int thr = 0;
  int ail = 0;
  int s1 = 0;
  int s2 = 0;
  int sa = 0;
  int sb = 0;
  int sc = 0;
  int sd = 0;
  quint64 lsw = 0;
  int ch1 = 0;
  int ch2 = 0;
  int ch3 = 0;
  int ch4 = 0;
  int ch5 = 0;
  int ch6 = 0;
  int ch7 = 0;
  int ch8 = 0;
  double txBat = 0.0;
};

struct LogParseResult {
  QVector<LogRecord> records;
  QStringList headers;
  QVector<LogColumnId> columnIds;
  int errors = 0;
  int lines = 0;
  QString baseName;
};

Q_DECLARE_METATYPE(LogColumnId)
Q_DECLARE_METATYPE(LogRecord)
Q_DECLARE_METATYPE(QVector<LogRecord>)
Q_DECLARE_METATYPE(LogParseResult)
