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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include <QtCore>
#include <QDialog>
#include <QThread>
#include <chrono>
#include "qcustomplot.h"
#include "logrecord.h"

#define INVALID_MIN 999999
#define INVALID_MAX -999999

namespace Ui {
  class LogsDialog;
}

class LogTableModel;
class LogFileParserWorker;

class LogsDialog : public QDialog
{
  Q_OBJECT

  enum yaxes_t {
    firstLeft = 0,
    firstRight,
    secondLeft,
    secondRight,
    AXES_LIMIT // = 4
  };

  struct coords_t {
    QVector<double> x, y;
    double min_y;
    double max_y;
    yaxes_t yaxis;
    QString name;
  };

  struct minMax_t {
    double min;
    double max;
  };

  struct plotsCollection {
    QVarLengthArray<coords_t> coords;
    double min_x;
    double max_x;
    bool tooManyRanges;
  };

public:
  explicit LogsDialog(QWidget *parent = 0);
  ~LogsDialog();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void titleDoubleClicked(QMouseEvent *evt);
  void axisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
  void legendDoubleClick(QCPLegend* legend, QCPAbstractLegendItem* item);
  void selectionChanged();
  void mousePress(QMouseEvent * event);
  void mouseWheel();
  void removeAllGraphs();
  void plotLogs();
  void fileOpen();
  void saveSession();
  void sessionsCurrentIndexChanged(int index);
  void mapsButtonClicked();
  void yAxisChangeRanges(QCPRange range);
  std::pair<double, double> GetMinMaxY() const;
  void onParseProgress(int percent);
  void onParseFinished(const LogParseResult & result);
  void onParseFailed(const QString & error);
  void onParseCanceled();

private:
  QVector<LogRecord> m_records;
  QStringList m_headers;
  QVector<LogColumnId> m_columnIds;
  Ui::LogsDialog *ui;
  LogTableModel * m_logModel = nullptr;
  QCPAxisRect *axisRect;
  QCPLegend *rightLegend;
  bool plotLock;
  QString logFilename;

  // Load timing: from QFile::open() until the log table has been painted.
  std::chrono::steady_clock::time_point m_loadStart;
  bool m_loadTimingActive = false;
  bool m_tableModelReady = false;
  QString m_timingFile;
  void writeLoadTiming(double dtMs, const char * status) const;

  QVarLengthArray<Qt::GlobalColor> colors;
  QPen pen;

  double yAxesRatios[AXES_LIMIT];
  minMax_t yAxesRanges[AXES_LIMIT];

  QCPItemTracer * tracerMaxAlt;
  QCPItemTracer * cursorA;
  QCPItemTracer * cursorB;
  QCPItemStraightLine * cursorLine;

  bool cvsFileParse();
  void startParseFile(const QString & fileName);
  void stopParseWorker();
  void setParseUiState(bool active);
  void finishParseUiState();
  void applyParsedLog(const LogParseResult & result);
  QVector<LogRecord> filterGePoints(const QVector<LogRecord> & input) const;
  void exportToGoogleEarth();
  QDateTime getRecordTimeStamp(int index);
  QString generateDuration(const QDateTime & start, const QDateTime & end);
  void setFlightSessions();

  void addMaxAltitudeMarker(const coords_t & c, QCPGraph * graph);
  void countNumberOfThrows(const coords_t & c, QCPGraph * graph);
  void addCursor(QCPItemTracer ** cursor, QCPGraph * graph, const QColor & color);
  void addCursorLine(QCPItemStraightLine ** line, QCPGraph * graph, const QColor & color);
  void placeCursor(double x, bool second);
  QString formatTimeDelta(double timeDelta);
  void updateCursorsLabel();
  void updateStatsPanel();

  QThread * m_parseThread = nullptr;
  LogFileParserWorker * m_parseWorker = nullptr;
  QString m_pendingLogFile;
  bool m_parseCanceled = false;


};
