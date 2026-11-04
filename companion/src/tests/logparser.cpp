#include "gtests.h"
#include "logparser.h"
#include "location.h"
#include <QFile>

namespace {

QString loadTestFile(const QString & name)
{
  QFile file(QString(TESTS_DATA_PATH "/") + name);
  if (!file.open(QFile::ReadOnly | QFile::Text))
    return QString();
  return QString::fromLatin1(file.readAll());
}

}

TEST(LogParser, ParsesValidCsv)
{
  const QString input = loadTestFile("log_valid.csv");
  ASSERT_FALSE(input.isEmpty());
  LogParseResult result = parseLogCsvText(input);

  ASSERT_EQ(0, result.errors);
  ASSERT_EQ(1, result.records.size());
  EXPECT_EQ(1, result.lines);

  const LogRecord & rec = result.records.at(0);
  EXPECT_EQ(QDate(2024, 6, 1), rec.date);
  EXPECT_EQ(QTime(10, 0, 0, 0), rec.time);
  EXPECT_EQ(-102, rec.rss1);
  EXPECT_EQ(76, rec.rqly);
  EXPECT_EQ(-2, rec.rsnr);
  EXPECT_NEAR(7.8, rec.rxBt, 0.001);
  EXPECT_NEAR(0.1, rec.curr, 0.001);
  EXPECT_EQ(0, rec.capa);
  EXPECT_NEAR(39.3, rec.alt, 0.001);
  EXPECT_NEAR(3.0, rec.vspd, 0.001);
  EXPECT_NEAR(189.0, rec.gAlt, 0.001);
  EXPECT_NEAR(40.0, rec.gSpd, 0.001);
  EXPECT_EQ(0, rec.hdg);
  EXPECT_EQ(13, rec.sats);
  EXPECT_EQ(QString("ANGLE"), rec.fm);
  EXPECT_EQ(0, rec.rud);
  EXPECT_EQ(861, rec.ele);
  EXPECT_EQ(931, rec.thr);
  EXPECT_EQ(144, rec.ail);
  EXPECT_EQ(-774, rec.s1);
  EXPECT_EQ(-981, rec.s2);
  EXPECT_EQ(0, rec.sa);
  EXPECT_EQ(0, rec.sb);
  EXPECT_EQ(1, rec.sc);
  EXPECT_EQ(0, rec.sd);
  EXPECT_EQ(static_cast<quint64>(0), rec.lsw);
  EXPECT_EQ(1500, rec.ch1);
  EXPECT_EQ(1920, rec.ch2);
  EXPECT_EQ(1954, rec.ch3);
  EXPECT_EQ(1570, rec.ch4);
  EXPECT_EQ(1122, rec.ch5);
  EXPECT_EQ(1021, rec.ch6);
  EXPECT_EQ(1361, rec.ch7);
  EXPECT_EQ(1828, rec.ch8);
  EXPECT_NEAR(8.2, rec.txBat, 0.001);
}

TEST(LogParser, RejectsInvalidHeader)
{
  const QString input = loadTestFile("log_bad_header.csv");
  ASSERT_FALSE(input.isEmpty());
  LogParseResult result = parseLogCsvText(input);

  EXPECT_EQ(0, result.records.size());
  EXPECT_GT(result.errors, 0);
}

TEST(LogParser, CountsMalformedRow)
{
  const QString input = loadTestFile("log_short_row.csv");
  ASSERT_FALSE(input.isEmpty());
  LogParseResult result = parseLogCsvText(input);

  EXPECT_EQ(1, result.records.size());
  EXPECT_EQ(1, result.errors);
}

TEST(LogParser, RejectsInvalidValues)
{
  const QString input = loadTestFile("log_bad_value.csv");
  ASSERT_FALSE(input.isEmpty());
  LogParseResult result = parseLogCsvText(input);

  EXPECT_EQ(0, result.records.size());
  EXPECT_EQ(1, result.errors);
}

TEST(LogParser, RejectsInvalidDate)
{
  const QString input = loadTestFile("log_bad_date.csv");
  ASSERT_FALSE(input.isEmpty());
  LogParseResult result = parseLogCsvText(input);

  EXPECT_EQ(0, result.records.size());
  EXPECT_EQ(1, result.errors);
}
