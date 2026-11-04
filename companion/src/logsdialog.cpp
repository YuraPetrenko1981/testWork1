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

#include <math.h>
#include <cmath>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <QEvent>
#include <QItemSelectionModel>
#include <QFileInfo>
#include <QThread>
#include <QProgressBar>
#include <QPushButton>
#include "logsdialog.h"
#include "logtablemodel.h"
#include "logrecord.h"
#include "logparser.h"
#include "appdata.h"
#include "ui_logsdialog.h"
#include "helpers.h"
#if defined _MSC_VER || !defined __GNUC__
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {

QString formatDouble(double value)
{
  return QString::number(value, 'f', 3);
}

QString formatStatValue(double value, bool isInt)
{
  if (isInt)
    return QString::number(static_cast<qint64>(std::llround(value)));
  return formatDouble(value);
}

bool numericFieldValue(const LogRecord & record, LogColumnId id, double & value)
{
  switch (id) {
    case LogColumnId::EpochMs:
      value = static_cast<double>(record.epochMs);
      return true;
    case LogColumnId::Rss1:
      value = record.rss1;
      return true;
    case LogColumnId::RQly:
      value = record.rqly;
      return true;
    case LogColumnId::RSNR:
      value = record.rsnr;
      return true;
    case LogColumnId::RxBt:
      value = record.rxBt;
      return true;
    case LogColumnId::Curr:
      value = record.curr;
      return true;
    case LogColumnId::Capa:
      value = record.capa;
      return true;
    case LogColumnId::Alt:
      value = record.alt;
      return true;
    case LogColumnId::VSpd:
      value = record.vspd;
      return true;
    case LogColumnId::GAlt:
      value = record.gAlt;
      return true;
    case LogColumnId::GSpd:
      value = record.gSpd;
      return true;
    case LogColumnId::Hdg:
      value = record.hdg;
      return true;
    case LogColumnId::Sats:
      value = record.sats;
      return true;
    case LogColumnId::Rud:
      value = record.rud;
      return true;
    case LogColumnId::Ele:
      value = record.ele;
      return true;
    case LogColumnId::Thr:
      value = record.thr;
      return true;
    case LogColumnId::Ail:
      value = record.ail;
      return true;
    case LogColumnId::S1:
      value = record.s1;
      return true;
    case LogColumnId::S2:
      value = record.s2;
      return true;
    case LogColumnId::SA:
      value = record.sa;
      return true;
    case LogColumnId::SB:
      value = record.sb;
      return true;
    case LogColumnId::SC:
      value = record.sc;
      return true;
    case LogColumnId::SD:
      value = record.sd;
      return true;
    case LogColumnId::LSW:
      value = static_cast<double>(record.lsw);
      return true;
    case LogColumnId::CH1:
      value = record.ch1;
      return true;
    case LogColumnId::CH2:
      value = record.ch2;
      return true;
    case LogColumnId::CH3:
      value = record.ch3;
      return true;
    case LogColumnId::CH4:
      value = record.ch4;
      return true;
    case LogColumnId::CH5:
      value = record.ch5;
      return true;
    case LogColumnId::CH6:
      value = record.ch6;
      return true;
    case LogColumnId::CH7:
      value = record.ch7;
      return true;
    case LogColumnId::CH8:
      value = record.ch8;
      return true;
    case LogColumnId::TxBat:
      value = record.txBat;
      return true;
    case LogColumnId::Date:
    case LogColumnId::Time:
    case LogColumnId::GPS:
    case LogColumnId::FM:
    case LogColumnId::Unknown:
    default:
      return false;
  }
}

bool isIntegerField(LogColumnId id)
{
  switch (id) {
    case LogColumnId::EpochMs:
    case LogColumnId::Rss1:
    case LogColumnId::RQly:
    case LogColumnId::RSNR:
    case LogColumnId::Capa:
    case LogColumnId::Rud:
    case LogColumnId::Ele:
    case LogColumnId::Thr:
    case LogColumnId::Ail:
    case LogColumnId::S1:
    case LogColumnId::S2:
    case LogColumnId::SA:
    case LogColumnId::SB:
    case LogColumnId::SC:
    case LogColumnId::SD:
    case LogColumnId::CH1:
    case LogColumnId::CH2:
    case LogColumnId::CH3:
    case LogColumnId::CH4:
    case LogColumnId::CH5:
    case LogColumnId::CH6:
    case LogColumnId::CH7:
    case LogColumnId::CH8:
    case LogColumnId::Hdg:
    case LogColumnId::Sats:
      return true;
    case LogColumnId::LSW:
    case LogColumnId::RxBt:
    case LogColumnId::Curr:
    case LogColumnId::Alt:
    case LogColumnId::VSpd:
    case LogColumnId::GAlt:
    case LogColumnId::GSpd:
    case LogColumnId::TxBat:
    case LogColumnId::Date:
    case LogColumnId::Time:
    case LogColumnId::GPS:
    case LogColumnId::FM:
    case LogColumnId::Unknown:
    default:
      return false;
  }
}

}

class LogFileParserWorker : public QObject
{
  Q_OBJECT

public:
  explicit LogFileParserWorker(QObject *parent = nullptr) : QObject(parent) {}

  void requestCancel() { m_cancelRequested.store(true); }

public slots:
  void parseFile(const QString & fileName)
  {
    LogParseResult result;
    result.errors = 0;
    result.lines = -1;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      emit failed(tr("Cannot open file %1").arg(fileName));
      return;
    }

    const qint64 fileSize = file.size();
    qint64 bytesRead = 0;
    int lastPercent = -1;

    QByteArray fileBytes = file.readAll();
    bytesRead = fileBytes.size();

    if (m_cancelRequested.load()) {
      emit canceled();
      return;
    }

    result = parseLogCsvText(QString::fromLatin1(fileBytes));
    if (result.headers.isEmpty()) {
      emit failed(tr("Invalid log file format"));
      return;
    }

    if (m_cancelRequested.load()) {
      emit canceled();
      return;
    }

    if (fileSize > 0) {
      int percent = static_cast<int>((bytesRead * 100) / fileSize);
      if (percent != lastPercent) {
        lastPercent = percent;
        emit progress(percent);
      }
    }

    emit progress(100);
    result.baseName = QFileInfo(fileName).baseName();
    emit finished(result);
  }

signals:
  void progress(int percent);
  void finished(const LogParseResult & result);
  void failed(const QString & error);
  void canceled();

private:
  std::atomic<bool> m_cancelRequested{false};
};

LogsDialog::LogsDialog(QWidget *parent) :
  QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
  ui(new Ui::LogsDialog),
  tracerMaxAlt(0),
  cursorA(0),
  cursorB(0),
  cursorLine(0)
{

  ui->setupUi(this);
  setWindowIcon(CompanionIcon("logs.png"));

  m_logModel = new LogTableModel(this);
  ui->logTable->setModel(m_logModel);
  ui->logTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

  ui->logTable->viewport()->installEventFilter(this);

  plotLock=false;

  colors.append(Qt::green);
  colors.append(Qt::red);
  colors.append(Qt::magenta);
  colors.append(Qt::cyan);
  colors.append(Qt::blue);
  colors.append(Qt::yellow);
  colors.append(Qt::darkGreen);
  colors.append(Qt::darkRed);
  colors.append(Qt::darkMagenta);
  colors.append(Qt::darkCyan);
  colors.append(Qt::darkBlue);
  colors.append(Qt::darkYellow);
  pen.setWidthF(1.0);

  // create and prepare a plot title layout element
  QCPTextElement *title = new QCPTextElement(ui->customPlot);
  QFont titlefont = font();
  titlefont.setBold(true);
  title->setText(tr("Telemetry logs"));
  title->setFont(titlefont);

  // add it to the main plot layout
  ui->customPlot->plotLayout()->insertRow(0);
  ui->customPlot->plotLayout()->addElement(0, 0, title);
  ui->customPlot->setNoAntialiasingOnDrag(true);
  ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iSelectLegend | QCP::iSelectPlottables);

  axisRect = ui->customPlot->axisRect();

  // configure bottom axis to show time instead of number:
  axisRect->axis(QCPAxis::atBottom)->setLabel(tr("Time (hh:mm:ss.ms)"));
  QSharedPointer<QCPAxisTickerDateTime> timeTicker(new QCPAxisTickerDateTime);
  timeTicker->setDateTimeFormat("hh:mm:ss.zzz");
  axisRect->axis(QCPAxis::atBottom)->setTicker(timeTicker);
  QDateTime now = QDateTime::currentDateTime();
  axisRect->axis(QCPAxis::atBottom)->setRange(QCPAxisTickerDateTime::dateTimeToKey(now.addSecs(-60 * 60 * 2)),
                                              QCPAxisTickerDateTime::dateTimeToKey(now));
  axisRect->axis(QCPAxis::atLeft)->setTickLabels(false);
  axisRect->addAxis(QCPAxis::atLeft);
  axisRect->addAxis(QCPAxis::atRight);
  axisRect->axis(QCPAxis::atLeft, 1)->setVisible(false);
  axisRect->axis(QCPAxis::atRight, 1)->setVisible(false);

  QFont legendFont = font();
  legendFont.setPointSize(10);
  ui->customPlot->legend->setFont(legendFont);
  ui->customPlot->legend->setSelectedFont(legendFont);
  axisRect->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);

  rightLegend = new QCPLegend;
  axisRect->insetLayout()->addElement(rightLegend, Qt::AlignTop | Qt::AlignRight);
  rightLegend->setLayer("legend");
  rightLegend->setFont(legendFont);
  rightLegend->setSelectedFont(legendFont);
  rightLegend->setVisible(false);

  ui->customPlot->setAutoAddPlottableToLegend(false);

  QString path = g.gePath();
  if (path.isEmpty() || !QFile(path).exists()) {
    ui->mapsButton->hide();
  }

  ui->SaveSession_PB->setEnabled(false);

  // connect slot that ties some axis selections together (especially opposite axes):
  connect(ui->customPlot, &QCustomPlot::selectionChangedByUser, this, &LogsDialog::selectionChanged);
  // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
  connect(ui->customPlot, &QCustomPlot::mousePress, this, &LogsDialog::mousePress);
  connect(ui->customPlot, &QCustomPlot::mouseWheel, this, &LogsDialog::mouseWheel);

  // make left axes transfer its range to right axes:
  connect(axisRect->axis(QCPAxis::atLeft), static_cast<void(QCPAxis::*)(const QCPRange&)>(&QCPAxis::rangeChanged), this, &LogsDialog::yAxisChangeRanges);
  // connect some interaction slots:
  connect(title, &QCPTextElement::doubleClicked, this, &LogsDialog::titleDoubleClicked);
  connect(ui->customPlot, &QCustomPlot::axisDoubleClick, this, &LogsDialog::axisLabelDoubleClick);
  connect(ui->customPlot, &QCustomPlot::legendDoubleClick, this, &LogsDialog::legendDoubleClick);
  connect(ui->FieldsTW, &QTableWidget::itemSelectionChanged, this, &LogsDialog::plotLogs);
  connect(ui->logTable->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &LogsDialog::plotLogs);
  connect(axisRect->axis(QCPAxis::atBottom), static_cast<void(QCPAxis::*)(const QCPRange&)>(&QCPAxis::rangeChanged), this, &LogsDialog::updateStatsPanel);
  connect(ui->Reset_PB, &QPushButton::clicked, this, [this]() {
    ui->ZoomX_ChkB->setChecked(false);
    ui->ZoomY_ChkB->setChecked(false);
    ui->CommonAxes_ChkB->setChecked(false);
    plotLogs();
  });
  connect(ui->CommonAxes_ChkB, &QPushButton::clicked, this, &LogsDialog::plotLogs);
  connect(ui->SaveSession_PB, &QPushButton::clicked, this, &LogsDialog::saveSession);
  connect(ui->fileOpen_PB, &QPushButton::clicked, this, &LogsDialog::fileOpen);
  connect(ui->parseCancelButton, &QPushButton::clicked, this, [this]() {
    m_parseCanceled = true;
    if (m_parseWorker) {
      m_parseWorker->requestCancel();
    }
  });
  connect(ui->mapsButton, &QPushButton::clicked, this, &LogsDialog::mapsButtonClicked);
  connect(ui->sessions_CB, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LogsDialog::sessionsCurrentIndexChanged);
}

LogsDialog::~LogsDialog()
{
  stopParseWorker();
  delete ui;
}

void LogsDialog::titleDoubleClicked(QMouseEvent *evt)
{
  // Set the plot title by double clicking on it
  QCPTextElement *title = qobject_cast<QCPTextElement*>(sender());
  if (title) {
    bool ok;
    QString newTitle = QInputDialog::getText(this, tr("Plot Title Change"), tr("New plot title:"), QLineEdit::Normal, title->text(), &ok);
    if (ok) {
      title->setText(newTitle);
      ui->customPlot->replot();
    }
  }
}

void LogsDialog::axisLabelDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
  // Set an axis label by double clicking on it
  if (part == QCPAxis::spAxisLabel) {
    // only react when the actual axis label is clicked, not tick label or axis backbone
    bool ok;
    QString newLabel = QInputDialog::getText(this, tr("Axis Label Change"), tr("New axis label:"), QLineEdit::Normal, axis->label(), &ok);
    if (ok) {
      axis->setLabel(newLabel);
      ui->customPlot->replot();
    }
  }
}

void LogsDialog::legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item)
{
  // Rename a graph by double clicking on its legend item
  if (item) {
    // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
    QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Graph Name Change"), tr("New graph name:"), QLineEdit::Normal, plItem->plottable()->name(), &ok);
    if (ok) {
      plItem->plottable()->setName(newName);
      ui->customPlot->replot();
    }
  }
}

void LogsDialog::selectionChanged()
{
  /*
   normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
   the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
   and the axis base line together. However, the axis label shall be selectable individually.

   The selection state of the left and right axes shall be synchronized as well as the state of the
   bottom and top axes.

   Further, we want to synchronize the selection of the graphs with the selection state of the respective
   legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
   or on its legend item.
  */

  if (plotLock) return;

  // handle bottom axis and tick labels as one selectable object:
  if (axisRect->axis(QCPAxis::atBottom)->selectedParts().testFlag(QCPAxis::spAxis) ||
    axisRect->axis(QCPAxis::atBottom)->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    axisRect->axis(QCPAxis::atBottom)->setSelectedParts(QCPAxis::spAxis |
      QCPAxis::spTickLabels);
  }
  // make left and right axes be selected synchronously,
  // and handle axis and tick labels as one selectable object:
  if (axisRect->axis(QCPAxis::atLeft)->selectedParts().testFlag(QCPAxis::spAxis) ||
    axisRect->axis(QCPAxis::atLeft)->selectedParts().testFlag(QCPAxis::spTickLabels) ||
    (
      axisRect->axis(QCPAxis::atRight)->visible() &&
      (axisRect->axis(QCPAxis::atRight)->selectedParts().testFlag(QCPAxis::spAxis) ||
      axisRect->axis(QCPAxis::atRight)->selectedParts().testFlag(QCPAxis::spTickLabels))
    ) || (
      axisRect->axis(QCPAxis::atLeft, 1)->visible() &&
      (axisRect->axis(QCPAxis::atLeft, 1)->selectedParts().testFlag(QCPAxis::spAxis) ||
      axisRect->axis(QCPAxis::atLeft, 1)->selectedParts().testFlag(QCPAxis::spTickLabels))
    ) || (
      axisRect->axis(QCPAxis::atRight)->visible() &&
      (axisRect->axis(QCPAxis::atRight, 1)->selectedParts().testFlag(QCPAxis::spAxis) ||
      axisRect->axis(QCPAxis::atRight, 1)->selectedParts().testFlag(QCPAxis::spTickLabels))
    )
  ) {
    axisRect->axis(QCPAxis::atLeft)->setSelectedParts(QCPAxis::spAxis |
      QCPAxis::spTickLabels);
    if (axisRect->axis(QCPAxis::atRight)->visible()) {
      axisRect->axis(QCPAxis::atRight)->setSelectedParts(QCPAxis::spAxis |
        QCPAxis::spTickLabels);
      if (axisRect->axis(QCPAxis::atLeft, 1)->visible()) {
        axisRect->axis(QCPAxis::atLeft, 1)->setSelectedParts(QCPAxis::spAxis |
          QCPAxis::spTickLabels);
        if (axisRect->axis(QCPAxis::atRight, 1)->visible()) {
          axisRect->axis(QCPAxis::atRight, 1)->setSelectedParts(QCPAxis::spAxis |
            QCPAxis::spTickLabels);
        }
      }
    }
  }

  // synchronize selection of graphs with selection of corresponding legend items:
  for (int i=0; i<ui->customPlot->graphCount(); ++i) {
    QCPGraph *graph = ui->customPlot->graph(i);
    QCPPlottableLegendItem *item = ui->customPlot->legend->itemWithPlottable(graph);
    if (item == NULL) item = rightLegend->itemWithPlottable(graph);
    if (item->selected() || graph->selected()) {
      item->setSelected(true);
      graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
    }
  }
}

QVector<LogRecord> LogsDialog::filterGePoints(const QVector<LogRecord> & input) const
{
  QVector<LogRecord> result;

  int n = input.count();
  if (n == 0) {
    return result;
  }

  bool hasGps = false;
  for (LogColumnId id : m_columnIds) {
    if (id == LogColumnId::GPS) {
      hasGps = true;
      break;
    }
  }
  if (!hasGps) {
    QMessageBox::critical(const_cast<LogsDialog *>(this), tr("Error: no GPS data found"),
      tr("The column containing GPS coordinates must be named \"GPS\".\n\n\
The columns for altitude \"GAlt\" and for speed \"GSpd\" are optional"));
    return result;
  }

  bool rangeSelected = ui->logTable->selectionModel()->selectedRows().length() > 0;

  GpsGlitchFilter glitchFilter;
  GpsLatLonFilter latLonFilter;

  for (int i = 0; i < n; i++) {
    if ((ui->logTable->selectionModel()->isRowSelected(i, QModelIndex()) && rangeSelected) || !rangeSelected) {

      GpsCoord coord = extractGpsCoordinates(input.at(i).gps);

      // glitch filter
      if ( glitchFilter.isGlitch(coord) ) {
        // qDebug() << "filterGePoints(): GPS glitch detected at" << i << coord.latitude << coord.longitude;
        continue;
      }

      // lat long pair filter
      if ( !latLonFilter.isValid(coord) ) {
        // qDebug() << "filterGePoints(): Lat-Lon pair wrong, skipping at" << i << coord.latitude << coord.longitude;
        continue;
      }

      // qDebug() << "point " << latitude << longitude;
      result.append(input.at(i));
    }
  }

  // qDebug() << "filterGePoints(): filtered from" << input.count() << "to " << result.count() << "points";
  return result;
}

void LogsDialog::exportToGoogleEarth()
{
  // filter data points
  QVector<LogRecord> dataPoints = filterGePoints(m_records);
  int n = dataPoints.count(); // number of points to export
  if (n == 0) return;

  double altMultiplier = 1.0;

  QSet<int> extradataCols;

  for (int j = 0; j < ui->FieldsTW->rowCount(); j++) {
    if (ui->FieldsTW->item(j, 0) && ui->FieldsTW->item(j, 0)->isSelected()) {
      extradataCols << (j + 3);
    }
  }

  // qDebug() << "gpscol" << gpscol << "altcol" << altcol << "speedcol" << speedcol << "altMultiplier" << altMultiplier;
  const QString geFilename = generateProcessUniqueTempFileName("flight.kml");
  QFile geFile(geFilename);
  if (!geFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    QMessageBox::warning(this, CPN_STR_TTL_ERROR,
        tr("Cannot write file %1:\n%2.")
        .arg(geFilename)
        .arg(geFile.errorString()));
    return;
  }

  const QString geIconFilename = QStringLiteral("track0.png");
  const QString geIconPath = QFileInfo(geFile).absolutePath() + "/" + geIconFilename;
  if (!QFile::exists(geIconPath)) {
    QFile::copy(":/images/" + geIconFilename, geIconPath);
  }

  QTextStream outputStream(&geFile);

  // file header
  outputStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\">\n";
  outputStream << "\t<Document>\n\t\t<name>" << logFilename << "</name>\n";
  outputStream << "\t\t<Style id=\"multiTrack_n\">\n\t\t\t<IconStyle>\n\t\t\t\t<Icon>\n\t\t\t\t\t<href>" << geIconFilename << "</href>\n\t\t\t\t</Icon>\n\t\t\t</IconStyle>\n\t\t\t<LineStyle>\n\t\t\t\t<color>991081f4</color>\n\t\t\t\t<width>6</width>\n\t\t\t</LineStyle>\n\t\t</Style>\n";
  outputStream << "\t\t<Style id=\"multiTrack_h\">\n\t\t\t<IconStyle>\n\t\t\t\t<scale>0</scale>\n\t\t\t\t<Icon>\n\t\t\t\t\t<href>" << geIconFilename << "</href>\n\t\t\t\t</Icon>\n\t\t\t</IconStyle>\n\t\t\t<LineStyle>\n\t\t\t\t<color>991081f4</color>\n\t\t\t\t<width>8</width>\n\t\t\t</LineStyle>\n\t\t</Style>\n";
  outputStream << "\t\t<StyleMap id=\"multiTrack\">\n\t\t\t<Pair>\n\t\t\t\t<key>normal</key>\n\t\t\t\t<styleUrl>#multiTrack_n</styleUrl>\n\t\t\t</Pair>\n\t\t\t<Pair>\n\t\t\t\t<key>highlight</key>\n\t\t\t\t<styleUrl>#multiTrack_h</styleUrl>\n\t\t\t</Pair>\n\t\t</StyleMap>\n";
  outputStream << "\t\t<Style id=\"lineStyle\">\n\t\t\t<LineStyle>\n\t\t\t\t<color>991081f4</color>\n\t\t\t\t<width>6</width>\n\t\t\t</LineStyle>\n\t\t</Style>\n";
  outputStream << "\t\t<Schema id=\"schema\">\n";
  outputStream << "\t\t\t<gx:SimpleArrayField name=\"GPSSpeed\" type=\"float\">\n\t\t\t\t<displayName>GPS Speed</displayName>\n\t\t\t</gx:SimpleArrayField>\n";

  // declare additional fields
  for (auto i = extradataCols.cbegin(), end = extradataCols.cend(); i != end; ++i) {
    QString origName = m_headers.at(*i);
    QString safeName = origName;
    safeName.replace(" ","_");
    outputStream << "\t\t\t<gx:SimpleArrayField name=\""<< safeName <<"\" ";
    outputStream << "type=\"string\"";   // additional fields have fixed type: string
    outputStream << ">\n\t\t\t\t<displayName>" << origName << "</displayName>\n\t\t\t</gx:SimpleArrayField>\n";
  }

  QString planeName;
  if (logFilename.indexOf("-") > 0) {
    planeName = logFilename.left(logFilename.indexOf("-"));
  } else {
    planeName = logFilename;
  }

  outputStream << "\t\t</Schema>\n";
  outputStream << "\t\t<Folder>\n\t\t\t<name>Log Data</name>\n\t\t\t<Placemark>\n\t\t\t\t<name>" << planeName << "</name>";
  outputStream << "\n\t\t\t\t<styleUrl>#multiTrack</styleUrl>";
  outputStream << "\n\t\t\t\t<gx:Track>\n";
  outputStream << "\n\t\t\t\t\t<altitudeMode>absolute</altitudeMode>\n";

  // date and time data points
  for (int i = 0; i < n; i++) {
    QString tstamp = dataPoints.at(i).date.toString("yyyy-MM-dd") +
      QString("T") + dataPoints.at(i).time.toString("hh:mm:ss.zzz") + QString("Z");
    outputStream << "\t\t\t\t\t<when>"<< tstamp <<"</when>\n";
  }

  // coordinate data points
  outputStream.setRealNumberNotation(QTextStream::FixedNotation);
  outputStream.setRealNumberPrecision(8);

  for (int i = 0; i < n; i++) {
    GpsCoord coord = extractGpsCoordinates(dataPoints.at(i).gps);
    int altitude = static_cast<int>(dataPoints.at(i).gAlt * altMultiplier);
    outputStream << "\t\t\t\t\t<gx:coord>" << coord.longitude << " " << coord.latitude << " " << altitude << " </gx:coord>\n" ;
  }

  // additional data for data points
  outputStream << "\t\t\t\t\t<ExtendedData>\n\t\t\t\t\t\t<SchemaData schemaUrl=\"#schema\">\n";

  // gps speed data points
  outputStream << "\t\t\t\t\t\t\t<gx:SimpleArrayData name=\"GPSSpeed\">\n";
  for (int i = 0; i < n; i++) {
    outputStream << "\t\t\t\t\t\t\t\t<gx:value>"<< dataPoints.at(i).gSpd <<"</gx:value>\n";
  }
  outputStream << "\t\t\t\t\t\t\t</gx:SimpleArrayData>\n";

  // add values for additional fields
  for (auto i = extradataCols.cbegin(), end = extradataCols.cend(); i != end; ++i) {
    QString safeName = m_headers.at(*i);
    safeName.replace(" ","_");
    outputStream << "\t\t\t\t\t\t\t<gx:SimpleArrayData name=\""<< safeName <<"\">\n";

    for (int j = 0; j < n; j++) {
      outputStream << "\t\t\t\t\t\t\t\t<gx:value>"<< m_logModel->textAt(j, *i) <<"</gx:value>\n";
    }

    outputStream << "\t\t\t\t\t\t\t</gx:SimpleArrayData>\n";
  }

  outputStream << "\t\t\t\t\t\t</SchemaData>\n\t\t\t\t\t</ExtendedData>\n\t\t\t\t</gx:Track>\n\t\t\t</Placemark>\n\t\t</Folder>\n\t</Document>\n</kml>";
  geFile.close();

  QString gePath = g.gePath();
  QStringList parameters;
#ifdef __APPLE__
  parameters << "-a";
  parameters << gePath;
  gePath = "/usr/bin/open";
#endif
  parameters << geFilename;
  QProcess::startDetached(gePath, parameters);
}

void LogsDialog::mapsButtonClicked()
{
  ui->FieldsTW->setDisabled(true);
  ui->logTable->setDisabled(true);

  exportToGoogleEarth();

  ui->FieldsTW->setDisabled(false);
  ui->logTable->setDisabled(false);
}

void LogsDialog::mousePress(QMouseEvent * event)
{
  // if an axis is selected, only allow the direction of that axis to be dragged
  // if no axis is selected, both directions may be dragged

  if (axisRect->axis(QCPAxis::atBottom)->selectedParts().testFlag(QCPAxis::spAxis))
    axisRect->setRangeDrag(axisRect->axis(QCPAxis::atBottom)->orientation());
  else if (axisRect->axis(QCPAxis::atLeft)->selectedParts().testFlag(QCPAxis::spAxis))
    axisRect->setRangeDrag(axisRect->axis(QCPAxis::atLeft)->orientation());
  else
    axisRect->setRangeDrag(Qt::Horizontal | Qt::Vertical);

  if (event->button() == Qt::RightButton) {
    double x = axisRect->axis(QCPAxis::atBottom)->pixelToCoord(event->pos().x());
    placeCursor(x, event->modifiers() & Qt::ShiftModifier);
  }
}

void LogsDialog::placeCursor(double x, bool second)
{
  QCPItemTracer * cursor = second ? cursorB : cursorA;

  if (cursor) {
    cursor->setGraphKey(x);
    cursor->updatePosition();
    cursor->setVisible(true);
  }

  if (cursorA && cursorB) {
    updateCursorsLabel();
  }
}

void LogsDialog::updateCursorsLabel()
{
  QString text = QString("Max Altitude: %1 m\n").arg(tracerMaxAlt->position->value(), 0, 'f', 1);
  if (cursorA->visible() ) {
    text += tr("Cursor A: %1 m").arg(cursorA->position->value(), 0, 'f', 1) + "\n";
  }
  if (cursorB->visible() ) {
    text += tr("Cursor B: %1 m").arg(cursorB->position->value(), 0, 'f', 1) + "\n";
  }
  if (cursorA->visible() && cursorB->visible()) {
    cursorLine->setVisible(true);
    // calc deltas
    double deltaX = cursorB->position->key() - cursorA->position->key();
    double deltaY = cursorB->position->value() - cursorA->position->value();
    double slope = 0;
    if (deltaX != 0) {
      slope = deltaY / deltaX;
    }
    qDebug() << "Cursors: dt:" << formatTimeDelta(deltaX) << "dy:" << deltaY << "rate:" << slope;
    text += tr("Time delta: %1").arg(formatTimeDelta(deltaX)) + "\n";
    text += tr("Climb rate: %1 m/s").arg(slope, 0, 'f', fabs(slope)<1.0 ? 2 : 1) + "\n";
  }
  ui->labelCursors->setText(text);
}

QString LogsDialog::formatTimeDelta(double timeDelta)
{
  if (abs(int(timeDelta)) < 10) {
    return QString("%1 s").arg(timeDelta, 1, 'f', 1);
  }

  int seconds = (int)round(fabs(timeDelta));
  int hours = seconds / 3600;
  seconds %= 3600;
  int minutes = seconds / 60;
  seconds %= 60;

  if (hours) {
    return QString("%1h:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
  }
  else if (minutes) {
    return QString("%1m:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
  }
  else {
    return QString("%1 s").arg(seconds);
  }
}

void LogsDialog::mouseWheel()
{
  // if an axis is selected, only allow the direction of that axis to be zoomed
  // if no axis is selected, both directions may be zoomed
  int orientation=0;
  if (ui->ZoomX_ChkB->isChecked()) {
    orientation|=Qt::Horizontal;
  }
  if (ui->ZoomY_ChkB->isChecked()) {
    orientation|=Qt::Vertical;
  }
  if (orientation) {
    axisRect->setRangeZoom((Qt::Orientation)orientation);
  } else {
    axisRect->setRangeZoom(Qt::Horizontal|Qt::Vertical);
  }
}

void LogsDialog::removeAllGraphs()
{
  ui->customPlot->clearGraphs();
  ui->customPlot->clearItems();
  ui->customPlot->legend->setVisible(false);
  rightLegend->clearItems();
  rightLegend->setVisible(false);
  axisRect->axis(QCPAxis::atRight)->setSelectedParts(QCPAxis::spNone);
  axisRect->axis(QCPAxis::atRight)->setVisible(false);
  axisRect->axis(QCPAxis::atLeft)->setSelectedParts(QCPAxis::spNone);
  axisRect->axis(QCPAxis::atLeft)->setTickLabels(false);
  axisRect->axis(QCPAxis::atLeft, 1)->setVisible(false);
  axisRect->axis(QCPAxis::atLeft, 1)->setSelectedParts(QCPAxis::spNone);
  axisRect->axis(QCPAxis::atRight, 1)->setVisible(false);
  axisRect->axis(QCPAxis::atRight, 1)->setSelectedParts(QCPAxis::spNone);
  axisRect->axis(QCPAxis::atBottom)->setSelectedParts(QCPAxis::spNone);
  ui->customPlot->replot();
  tracerMaxAlt = 0;
  cursorA = 0;
  cursorB = 0;
  cursorLine = 0;
  ui->labelCursors->setText("");
}

void LogsDialog::fileOpen()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select your log file"), g.logDir());
  if (!fileName.isEmpty()) {
    g.logDir(fileName);
    ui->FileName_LE->setText(fileName);
    startParseFile(fileName);
  }
}

void LogsDialog::startParseFile(const QString & fileName)
{
  stopParseWorker();
  m_pendingLogFile = fileName;
  m_parseCanceled = false;

  m_loadStart = std::chrono::steady_clock::now();
  m_loadTimingActive = true;
  m_tableModelReady = false;

  setParseUiState(true);

  m_parseThread = new QThread(this);
  m_parseWorker = new LogFileParserWorker();
  m_parseWorker->moveToThread(m_parseThread);

  connect(m_parseThread, &QThread::started, m_parseWorker, [this]() {
    m_parseWorker->parseFile(m_pendingLogFile);
  });
  connect(m_parseWorker, &LogFileParserWorker::progress, this, &LogsDialog::onParseProgress);
  connect(m_parseWorker, &LogFileParserWorker::finished, this, &LogsDialog::onParseFinished);
  connect(m_parseWorker, &LogFileParserWorker::failed, this, &LogsDialog::onParseFailed);
  connect(m_parseWorker, &LogFileParserWorker::canceled, this, &LogsDialog::onParseCanceled);
  connect(m_parseThread, &QThread::finished, m_parseWorker, &QObject::deleteLater);

  m_parseThread->start();
}

void LogsDialog::stopParseWorker()
{
  if (!m_parseThread)
    return;

  if (m_parseWorker) {
    m_parseWorker->requestCancel();
  }

  m_parseThread->quit();
  m_parseThread->wait();
  m_parseThread->deleteLater();
  m_parseThread = nullptr;
  m_parseWorker = nullptr;
}

void LogsDialog::setParseUiState(bool active)
{
  ui->parseCancelButton->setEnabled(active);
  if (active) {
    ui->parseProgressBar->setValue(0);
  }
  ui->fileOpen_PB->setEnabled(!active);
  ui->FieldsTW->setEnabled(!active);
  ui->logTable->setEnabled(!active);
  ui->mapsButton->setEnabled(!active);
  if (active) {
    ui->SaveSession_PB->setEnabled(false);
  }
}

void LogsDialog::finishParseUiState()
{
  ui->fileOpen_PB->setEnabled(true);
  ui->FieldsTW->setEnabled(true);
  ui->logTable->setEnabled(true);
  ui->mapsButton->setEnabled(true);
  ui->parseCancelButton->setEnabled(false);
}

void LogsDialog::onParseProgress(int percent)
{
  ui->parseProgressBar->setValue(percent);
}

void LogsDialog::onParseFinished(const LogParseResult & result)
{
  stopParseWorker();
  finishParseUiState();
  applyParsedLog(result);
}

void LogsDialog::onParseFailed(const QString & error)
{
  stopParseWorker();
  finishParseUiState();
  m_loadTimingActive = false;
  m_tableModelReady = false;
  QMessageBox::warning(this, CPN_STR_APP_NAME, error);
}

void LogsDialog::onParseCanceled()
{
  stopParseWorker();
  finishParseUiState();
  m_loadTimingActive = false;
  m_tableModelReady = false;
}

void LogsDialog::applyParsedLog(const LogParseResult & result)
{
  m_records = result.records;
  m_headers = result.headers;
  m_columnIds = result.columnIds;
  logFilename = result.baseName;

  if (m_records.isEmpty() || m_headers.isEmpty()) {
    m_loadTimingActive = false;
    m_tableModelReady = false;
    return;
  }

  ui->FieldsTW->clear();
  m_logModel->clear();
  ui->FieldsTW->setShowGrid(false);
  ui->FieldsTW->setContentsMargins(0, 0, 0, 0);
  ui->FieldsTW->setRowCount(m_headers.count() - 2);
  ui->FieldsTW->setColumnCount(1);
  ui->FieldsTW->setHorizontalHeaderLabels(QStringList(tr("Available fields")));
  ui->logTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  for (int i = 3; i < m_headers.count(); i++) {
    QTableWidgetItem* item= new QTableWidgetItem(m_headers.at(i));
    ui->FieldsTW->setItem(i - 3, 0, item);
  }

  ui->FieldsTW->resizeRowsToContents();
  m_logModel->setLog(&m_records, m_headers, m_columnIds);

  ui->logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  QVarLengthArray<int> sizes;

  for (int i = 0; i < m_logModel->columnCount(); i++) {
    sizes.append(ui->logTable->columnWidth(i));
  }

  ui->logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

  for (int i = 0; i < m_logModel->columnCount(); i++) {
    if (i == static_cast<int>(LogColumnId::EpochMs))
      ui->logTable->setColumnHidden(i, true); // hide MSecsSinceEpoch column
    else
      ui->logTable->setColumnWidth(i, sizes.at(i));
  }

  if (result.errors > 1) {
    QMessageBox::warning(this, CPN_STR_APP_NAME,
        tr("The selected logfile contains %1 invalid lines out of  %2 total lines").arg(result.errors).arg(result.lines));
  }

  plotLock = true;
  setFlightSessions();
  plotLock = false;

  // The model is fully populated; the next paint of the viewport is the
  // moment the table becomes visible.
  m_tableModelReady = true;
  ui->logTable->viewport()->update();
}

#include "logsdialog.moc"

bool LogsDialog::eventFilter(QObject *watched, QEvent *event)
{
  if (m_loadTimingActive && m_tableModelReady &&
      watched == ui->logTable->viewport() &&
      event->type() == QEvent::Paint) {

    const auto t1 = std::chrono::steady_clock::now();
    const double dtMs =
        std::chrono::duration<double, std::milli>(t1 - m_loadStart).count();

    writeLoadTiming(dtMs, "ok");

    m_loadTimingActive = false;
    m_tableModelReady = false;
  }

  return QDialog::eventFilter(watched, event);
}

void LogsDialog::writeLoadTiming(double dtMs, const char * status) const
{
  std::filesystem::path path;
#ifdef _WIN32
  if (const wchar_t *env = _wgetenv(L"CPN_TIMING_LOG"))
    path = env;
  else
#endif
    path = std::filesystem::temp_directory_path() / "companion_table_timing.txt";

  std::ofstream out(path, std::ios::app);
  if (!out)
    return;

  const auto wallMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

  out << std::fixed << std::setprecision(3)
      << "utc_ms=" << wallMs
      << " dt_ms=" << dtMs
      << " status=" << status
      << " rows=" << m_records.size()
      << " cols=" << m_headers.size()
      << " file=" << m_timingFile.toUtf8().constData()
      << '\n';
  out.flush();
}

void LogsDialog::saveSession()
{
  int index = ui->sessions_CB->currentIndex();
  // ignore index 0 is its all sessions combined
  if(index > 0) {
    int n = m_records.count();
    QVector<LogRecord> sessionRecords;
    // find session breaks
    int currentSession = 0;
    QDateTime lastvalue;
    for (int i = 0; i < n; i++) {
      QDateTime tmp = getRecordTimeStamp(i);
      if (!lastvalue.isValid() || lastvalue.secsTo(tmp) > 60) {
        currentSession++;
      }
      lastvalue = tmp;
      if(currentSession == index) {
        // add records to filtered list
        sessionRecords.append(m_records.at(i));
      }
      else if (currentSession > index) {
        break;
      }
    }
    // save the filtered records to a new file
    QString newFilename = logFilename;
    newFilename.append(QString("-Session%1.csv").arg(index));
    QString filename = QFileDialog::getSaveFileName(this, "Save log", newFilename, "CSV files (.csv);"); // getting the filename (full path)
    QFile data(filename);
    if(data.open(QFile::WriteOnly |QFile::Truncate)) {
      QTextStream output(&data);
      QStringList exportHeaders;
      exportHeaders.reserve(m_headers.size());
      for (const QString & header : m_headers) {
        if (header == "MSecsSinceEpoch")
          continue;
        exportHeaders.append(header);
      }

      output << exportHeaders.join(",") << '\n';

      for (const LogRecord &rec : sessionRecords) {
        QStringList row;
        row.reserve(exportHeaders.size());

        for (int i = 0; i < m_columnIds.size(); ++i) {
          if (m_columnIds.at(i) == LogColumnId::EpochMs)
            continue;

          switch (m_columnIds.at(i)) {
            case LogColumnId::Date:
              row << rec.date.toString("yyyy-MM-dd");
              break;
            case LogColumnId::Time:
              row << rec.time.toString("hh:mm:ss.zzz");
              break;
            case LogColumnId::Rss1:
              row << QString::number(rec.rss1);
              break;
            case LogColumnId::RQly:
              row << QString::number(rec.rqly);
              break;
            case LogColumnId::RSNR:
              row << QString::number(rec.rsnr);
              break;
            case LogColumnId::RxBt:
              row << formatDouble(rec.rxBt);
              break;
            case LogColumnId::Curr:
              row << formatDouble(rec.curr);
              break;
            case LogColumnId::Capa:
              row << QString::number(rec.capa);
              break;
            case LogColumnId::Alt:
              row << formatDouble(rec.alt);
              break;
            case LogColumnId::VSpd:
              row << formatDouble(rec.vspd);
              break;
            case LogColumnId::GPS:
              row << rec.gps;
              break;
            case LogColumnId::GAlt:
              row << formatDouble(rec.gAlt);
              break;
            case LogColumnId::GSpd:
              row << formatDouble(rec.gSpd);
              break;
            case LogColumnId::Hdg:
              row << QString::number(rec.hdg);
              break;
            case LogColumnId::Sats:
              row << QString::number(rec.sats);
              break;
            case LogColumnId::FM:
              row << rec.fm;
              break;
            case LogColumnId::Rud:
              row << QString::number(rec.rud);
              break;
            case LogColumnId::Ele:
              row << QString::number(rec.ele);
              break;
            case LogColumnId::Thr:
              row << QString::number(rec.thr);
              break;
            case LogColumnId::Ail:
              row << QString::number(rec.ail);
              break;
            case LogColumnId::S1:
              row << QString::number(rec.s1);
              break;
            case LogColumnId::S2:
              row << QString::number(rec.s2);
              break;
            case LogColumnId::SA:
              row << QString::number(rec.sa);
              break;
            case LogColumnId::SB:
              row << QString::number(rec.sb);
              break;
            case LogColumnId::SC:
              row << QString::number(rec.sc);
              break;
            case LogColumnId::SD:
              row << QString::number(rec.sd);
              break;
            case LogColumnId::LSW:
              row << QString("0x%1").arg(rec.lsw, 16, 16, QChar('0')).toUpper();
              break;
            case LogColumnId::CH1:
              row << QString::number(rec.ch1);
              break;
            case LogColumnId::CH2:
              row << QString::number(rec.ch2);
              break;
            case LogColumnId::CH3:
              row << QString::number(rec.ch3);
              break;
            case LogColumnId::CH4:
              row << QString::number(rec.ch4);
              break;
            case LogColumnId::CH5:
              row << QString::number(rec.ch5);
              break;
            case LogColumnId::CH6:
              row << QString::number(rec.ch6);
              break;
            case LogColumnId::CH7:
              row << QString::number(rec.ch7);
              break;
            case LogColumnId::CH8:
              row << QString::number(rec.ch8);
              break;
            case LogColumnId::TxBat:
              row << formatDouble(rec.txBat);
              break;
            case LogColumnId::EpochMs:
            case LogColumnId::Unknown:
            default:
              row << QString();
              break;
          }
        }

        output << row.join(",") << '\n';
      }
    }
    sessionRecords.clear();
  }
}

bool LogsDialog::cvsFileParse()
{
  return false;
}

struct FlightSession {
  QDateTime start;
  QDateTime end;
};

QDateTime LogsDialog::getRecordTimeStamp(int index)
{
  if (index < 0 || index >= m_records.size())
    return QDateTime();
  return QDateTime::fromMSecsSinceEpoch(m_records.at(index).epochMs);
}

QString LogsDialog::generateDuration(const QDateTime & start, const QDateTime & end)
{
  int secs = start.secsTo(end);
  QString durationString;
  if (secs >= 3600) {
    durationString = QString("%1:").arg(secs/3600);
    secs %= 3600;
  }
  durationString += QString("%1:%2").arg(secs/60, 2, 10, QChar('0')).arg(secs%60, 2, 10, QChar('0'));
  return durationString;
}

void LogsDialog::setFlightSessions()
{
  //Stopwatch s("Sessions");

  ui->sessions_CB->clear();
  ui->SaveSession_PB->setEnabled(false);

  int n = m_records.count();
  // qDebug() << "records" << n;

  // find session breaks
  QList<int> sessions;
  QDateTime lastvalue;
  for (int i = 1; i < n; i++) {
    QDateTime tmp = getRecordTimeStamp(i);
    if (!lastvalue.isValid() || lastvalue.secsTo(tmp) > 60) {
      sessions.push_back(i-1);
      // qDebug() << "session index" << i-1;
    }
    lastvalue = tmp;
  }
  sessions.push_back(n-1);

  //s.report("Breaks found");

  //now construct a list of sessions with their times
  //total time
  int noSesions = sessions.size() - 1;
  QString label = QString("%1 ").arg(noSesions);
  label += tr(noSesions > 1 ? "sessions" : "session");
  label += " <" + tr("time span ") + generateDuration(getRecordTimeStamp(1), getRecordTimeStamp(n - 1)) + ">";
  ui->sessions_CB->addItem(label);

  // add individual sessions
  if (sessions.size() > 2) {
    for (int i = 1; i < sessions.size(); i++) {
      QDateTime sessionStart = getRecordTimeStamp(sessions.at(i - 1) + 1);
      QDateTime sessionEnd = getRecordTimeStamp(sessions.at(i));
      QString label = sessionStart.toString("HH:mm:ss") + " <" + tr("duration ") + generateDuration(sessionStart, sessionEnd) + ">";
      ui->sessions_CB->addItem(label, sessions.at(i - 1));
      // qDebug() << "added label" << label << sessions.at(i-1);
    }
  }

  //s.report("List created");
}

void LogsDialog::sessionsCurrentIndexChanged(int index)
{
  if (plotLock) return;
  plotLock = true;

  ui->logTable->clearSelection();

  if (index != 0) {
    int bottom;
    if (index < ui->sessions_CB->count() - 1) {
      bottom = ui->sessions_CB->itemData(index + 1, Qt::UserRole).toInt();
    } else {
      bottom = m_logModel->rowCount();
    }

    QModelIndex topLeft = m_logModel->index(
      ui->sessions_CB->itemData(index, Qt::UserRole).toInt(), 0 , QModelIndex());
    QModelIndex bottomRight = m_logModel->index(
      bottom - 1, m_logModel->columnCount() - 1, QModelIndex());

    QItemSelection selection(topLeft, bottomRight);
    ui->logTable->selectionModel()->select(selection, QItemSelectionModel::Select);

    ui->SaveSession_PB->setEnabled(true);
  }
  else {
    ui->SaveSession_PB->setEnabled(false);
  }

  plotLock = false;
  plotLogs();
}

std::pair<double, double> LogsDialog::GetMinMaxY() const
{
  double minVal = 0.0;
  double maxVal = 0.0;
  bool found = false;

  // determine selected rows in the log table
  QModelIndexList sel = ui->logTable->selectionModel()->selectedRows();
  bool hasSelection = !sel.isEmpty();
  QVarLengthArray<int> selectedRows;

  if (hasSelection) {
    selectedRows.reserve(sel.size());
    for (const QModelIndex &idx : sel) selectedRows.append(idx.row());
  }

  // iterate over selected fields and rows to compute global min/max Y
  for (QTableWidgetItem *fieldItem : ui->FieldsTW->selectedItems()) {
    const int col = fieldItem->row() + 3;  // Epoch, Date and Time first
    const LogColumnId columnId = m_columnIds.value(col, LogColumnId::Unknown);
    const int rows = hasSelection ? selectedRows.length() : m_logModel->rowCount();

    for (int r = 0; r < rows; ++r) {
      const int tableRow = hasSelection ? selectedRows.at(r) : r;

      double v = 0.0;
      if (!numericFieldValue(m_records.at(tableRow), columnId, v))
        continue;

      if (!found) {
        minVal = maxVal = v;
        found = true;
      } else {
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
      }
    }
  }

  if (!found) return {0.0, 0.0};
  return {minVal, maxVal};
}

void LogsDialog::plotLogs()
{
  if (plotLock) return;

  if (!ui->FieldsTW->selectedItems().length()) {
    removeAllGraphs();
    updateStatsPanel();
    return;
  }

  //Stopwatch s("Plot");
  plotsCollection plots;

  QModelIndexList selection = ui->logTable->selectionModel()->selectedRows();
  int rowCount = selection.length();
  bool hasLogSelection;
  bool useCommonAxes = ui->CommonAxes_ChkB->isChecked();
  QVarLengthArray<int> selectedRows;

  if (rowCount) {
    hasLogSelection = true;
    foreach (QModelIndex index, selection) {
      selectedRows.append(index.row());
    }
    std::sort(selectedRows.begin(), selectedRows.end());
  } else {
    hasLogSelection = false;
    rowCount = m_logModel->rowCount();
  }

  //s.report("Row count");

  plots.min_x = INVALID_MIN;
  plots.max_x = 0;

  foreach (QTableWidgetItem *plot, ui->FieldsTW->selectedItems()) {
    coords_t plotCoords;
    int plotColumn = plot->row() + 3; // Epoch, Date and Time first
    const LogColumnId columnId = m_columnIds.value(plotColumn, LogColumnId::Unknown);

    plotCoords.min_y = INVALID_MIN;
    plotCoords.max_y = INVALID_MAX;
    plotCoords.yaxis = firstLeft;
    plotCoords.name = plot->text();
    bool hasY = false;

    for (int row = 0; row < rowCount; row++) {
      double y = 0.0;
      double time;
      QDateTime datetime;

      const int tableRow = hasLogSelection ? selectedRows.at(row) : row;

      if (!numericFieldValue(m_records.at(tableRow), columnId, y))
        continue;
      datetime = QDateTime::fromMSecsSinceEpoch(m_records.at(tableRow).epochMs);
      plotCoords.y.push_back(y);

      if (!hasY) {
        plotCoords.min_y = y;
        plotCoords.max_y = y;
        hasY = true;
      } else {
        if (plotCoords.min_y > y) plotCoords.min_y = y;
        if (plotCoords.max_y < y) plotCoords.max_y = y;
      }

      time = QCPAxisTickerDateTime::dateTimeToKey(datetime);

      plotCoords.x.push_back(time);

      if(plots.min_x == INVALID_MIN)
        plots.min_x = time;
      else
        if (plots.min_x > time) plots.min_x = time;

      if (plots.max_x < time) plots.max_x = time;
    }

    if (!hasY)
      continue;

    double range_inc = (plotCoords.max_y - plotCoords.min_y) / 100;
    if (range_inc == 0) range_inc = 1;
    plotCoords.max_y += range_inc;
    plotCoords.min_y -= range_inc;

    plots.coords.append(plotCoords);
  }

  //s.report("Cords");

  if (plots.coords.isEmpty()) {
    removeAllGraphs();
    updateStatsPanel();
    return;
  }

  yAxesRanges[firstLeft].min = plots.coords.at(0).min_y;
  yAxesRanges[firstLeft].max = plots.coords.at(0).max_y;
  for (int i = firstRight; i < AXES_LIMIT; i++) {
    yAxesRanges[i].min = INVALID_MIN;
    yAxesRanges[i].max = INVALID_MAX;
  }
  plots.tooManyRanges = false;

  for (int i = 1; i < plots.coords.size(); i++) {
    double actualRange = yAxesRanges[firstLeft].max - yAxesRanges[firstLeft].min;
    double thisRange = plots.coords.at(i).max_y - plots.coords.at(i).min_y;

    while (yAxesRanges[plots.coords.at(i).yaxis].max != INVALID_MAX &&
        (thisRange > actualRange * 1.3 || thisRange * 1.3 < actualRange ||
        plots.coords.at(i).min_y > yAxesRanges[plots.coords.at(i).yaxis].max ||
        plots.coords.at(i).max_y < yAxesRanges[plots.coords.at(i).yaxis].min)
      ) {

      if (!useCommonAxes) {
        switch (plots.coords[i].yaxis) {
          case firstLeft:
            plots.coords[i].yaxis = firstRight;
            break;
          case firstRight:
            plots.coords[i].yaxis = secondLeft;
            break;
          case secondLeft:
            plots.coords[i].yaxis = secondRight;
            break;
          case secondRight:
            plots.tooManyRanges = true;
            break;
          default:
            break;
        }
        if (plots.tooManyRanges) break;
      }

      actualRange = yAxesRanges[plots.coords.at(i).yaxis].max
        - yAxesRanges[plots.coords.at(i).yaxis].min;
    }

    if (plots.tooManyRanges) {
      break;
    } else {
      if (plots.coords.at(i).min_y < yAxesRanges[plots.coords.at(i).yaxis].min) {
        yAxesRanges[plots.coords.at(i).yaxis].min = plots.coords.at(i).min_y;
      }
      if (plots.coords.at(i).max_y > yAxesRanges[plots.coords.at(i).yaxis].max) {
        yAxesRanges[plots.coords.at(i).yaxis].max = plots.coords.at(i).max_y;
      }
    }
  }

  //s.report("Check too many ranges");

  if (plots.tooManyRanges) {
    yAxesRanges[firstLeft].max = 101;
    yAxesRanges[firstLeft].min = -1;
    yAxesRanges[firstRight].max = INVALID_MAX;
    yAxesRanges[firstRight].min = INVALID_MIN;
    yAxesRanges[secondLeft].max = INVALID_MAX;
    yAxesRanges[secondLeft].min = INVALID_MIN;
    yAxesRanges[secondRight].max = INVALID_MAX;
    yAxesRanges[secondRight].min = INVALID_MIN;

    for (int i = 0; i < plots.coords.size(); i++) {
      plots.coords[i].yaxis = firstLeft;

      double factor = 100 / (plots.coords.at(i).max_y - plots.coords.at(i).min_y);
      for (int j = 0; j < plots.coords.at(i).y.count(); j++) {
        plots.coords[i].y[j] = factor * (plots.coords.at(i).y.at(j) - plots.coords.at(i).min_y);
      }
    }
  } else {
    for (int i = firstRight; i < AXES_LIMIT; i++) {
      if (yAxesRanges[i].max == INVALID_MAX) break;

      yAxesRatios[i] = (yAxesRanges[i].max - yAxesRanges[i].min) /
        (yAxesRanges[firstLeft].max - yAxesRanges[firstLeft].min);
    }
  }

  removeAllGraphs();

  //s.report("Remove existing graphs");

  axisRect->axis(QCPAxis::atBottom)->setRange(plots.min_x, plots.max_x);
  if (useCommonAxes) {
    auto [min, max] = this->GetMinMaxY();
    axisRect->axis(QCPAxis::atLeft)->setRange(min, max);
  } else {
    axisRect->axis(QCPAxis::atLeft)->setRange(yAxesRanges[firstLeft].min, yAxesRanges[firstLeft].max);
  }

  if (plots.tooManyRanges) {
    axisRect->axis(QCPAxis::atLeft)->setTickLabels(false);
  } else {
    axisRect->axis(QCPAxis::atLeft)->setTickLabels(true);
  }

  if (yAxesRanges[firstRight].max != INVALID_MAX) {
    axisRect->axis(QCPAxis::atRight)->setRange(yAxesRanges[firstRight].min,
      yAxesRanges[firstRight].max);
    axisRect->axis(QCPAxis::atRight)->setVisible(true);

    rightLegend->setVisible(true);

    if (yAxesRanges[secondLeft].max != INVALID_MAX) {
      axisRect->axis(QCPAxis::atLeft, 1)->setVisible(true);
      axisRect->axis(QCPAxis::atLeft, 1)->setRange(yAxesRanges[secondLeft].min,
        yAxesRanges[secondLeft].max);

      if (yAxesRanges[secondRight].max != INVALID_MAX) {
        axisRect->axis(QCPAxis::atRight, 1)->setVisible(true);
        axisRect->axis(QCPAxis::atRight, 1)->setRange(yAxesRanges[secondRight].min,
          yAxesRanges[secondRight].max);
      }
    }
  }

  //s.report("Set axis");

  for (int i = 0; i < plots.coords.size(); i++) {
    switch (plots.coords[i].yaxis) {
      case firstLeft:
        ui->customPlot->addGraph();
        if (yAxesRanges[secondLeft].max != INVALID_MAX) {
          ui->customPlot->graph(i)->setName(plots.coords.at(i).name + tr(" (L1)"));
        } else {
          ui->customPlot->graph(i)->setName(plots.coords.at(i).name);
        }
        ui->customPlot->legend->addItem(
          new QCPPlottableLegendItem(ui->customPlot->legend, ui->customPlot->graph(i)));
        break;
      case firstRight:
        ui->customPlot->addGraph(axisRect->axis(QCPAxis::atBottom),
          axisRect->axis(QCPAxis::atRight));
        if (yAxesRanges[secondRight].max != INVALID_MAX) {
          ui->customPlot->graph(i)->setName(plots.coords.at(i).name + tr(" (R1)"));
        } else {
          ui->customPlot->graph(i)->setName(plots.coords.at(i).name);
        }
        rightLegend->addItem(
          new QCPPlottableLegendItem(rightLegend, ui->customPlot->graph(i)));
        break;
      case secondLeft:
        ui->customPlot->addGraph(axisRect->axis(QCPAxis::atBottom),
          axisRect->axis(QCPAxis::atLeft, 1));
        ui->customPlot->graph(i)->setName(plots.coords.at(i).name + tr(" (L2)"));
        ui->customPlot->legend->addItem(
          new QCPPlottableLegendItem(ui->customPlot->legend, ui->customPlot->graph(i)));
        break;
      case secondRight:
        ui->customPlot->addGraph(axisRect->axis(QCPAxis::atBottom),
          axisRect->axis(QCPAxis::atRight, 1));
        ui->customPlot->graph(i)->setName(plots.coords.at(i).name + tr(" (R2)"));
        rightLegend->addItem(
          new QCPPlottableLegendItem(rightLegend, ui->customPlot->graph(i)));
        break;
      default:
        break;
    }

    //s.report("Legend");

    ui->customPlot->graph(i)->setData(plots.coords.at(i).x,
      plots.coords.at(i).y);
    pen.setColor(colors.at(i % colors.size()));
    ui->customPlot->graph(i)->setPen(pen);

    if (!tracerMaxAlt && (plots.coords.at(i).name.endsWith("(m)") ||
        plots.coords.at(i).name.endsWith(" Alt") ||
        plots.coords.at(i).name.endsWith("(ft)"))) {
      addMaxAltitudeMarker(plots.coords.at(i), ui->customPlot->graph(i));
      countNumberOfThrows(plots.coords.at(i), ui->customPlot->graph(i));
      addCursor(&cursorA, ui->customPlot->graph(i), Qt::blue);
      addCursor(&cursorB, ui->customPlot->graph(i), Qt::red);
      addCursorLine(&cursorLine, ui->customPlot->graph(i), Qt::black);
      updateCursorsLabel();
    }
  }

  //s.report("Set graph");

  ui->customPlot->legend->setVisible(true);
  ui->customPlot->replot();
  updateStatsPanel();

  //s.report("Refresh graph");
}

void LogsDialog::updateStatsPanel()
{
  if (!ui->statsLabel)
    return;

  if (m_records.isEmpty() || m_columnIds.isEmpty()) {
    ui->statsLabel->setText(QString());
    return;
  }

  const QCPRange range = axisRect->axis(QCPAxis::atBottom)->range();
  const double minKey = range.lower;
  const double maxKey = range.upper;

  QStringList lines;

  QModelIndexList selection = ui->logTable->selectionModel()->selectedRows();
  QVarLengthArray<int> selectedRows;
  if (!selection.isEmpty()) {
    selectedRows.reserve(selection.size());
    for (const QModelIndex &idx : selection)
      selectedRows.append(idx.row());
  }

  for (QTableWidgetItem *fieldItem : ui->FieldsTW->selectedItems()) {
    const int col = fieldItem->row() + 3; // Epoch, Date and Time first
    const LogColumnId columnId = m_columnIds.value(col, LogColumnId::Unknown);
    if (columnId == LogColumnId::Unknown)
      continue;

    double minVal = 0.0;
    double maxVal = 0.0;
    double sumVal = 0.0;
    int count = 0;

    const int rowCount = selectedRows.isEmpty() ? m_records.size() : selectedRows.size();
    for (int i = 0; i < rowCount; ++i) {
      const int row = selectedRows.isEmpty() ? i : selectedRows.at(i);
      const LogRecord &rec = m_records.at(row);
      const double key = QCPAxisTickerDateTime::dateTimeToKey(QDateTime::fromMSecsSinceEpoch(rec.epochMs));
      if (key < minKey || key > maxKey)
        continue;

      double v = 0.0;
      if (!numericFieldValue(rec, columnId, v))
        continue;

      if (count == 0) {
        minVal = v;
        maxVal = v;
      } else {
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
      }
      sumVal += v;
      ++count;
    }

    if (count == 0) {
      lines << QString("%1: n/a").arg(fieldItem->text());
      continue;
    }

    const double avg = sumVal / count;
    const bool isInt = isIntegerField(columnId);
    lines << QString("%1: min=%2 max=%3 avg=%4")
      .arg(fieldItem->text())
      .arg(formatStatValue(minVal, isInt))
      .arg(formatStatValue(maxVal, isInt))
      .arg(formatStatValue(avg, isInt));
  }

  ui->statsLabel->setText(lines.join("\n"));
}

void LogsDialog::yAxisChangeRanges(QCPRange range)
{
  if (axisRect->axis(QCPAxis::atRight)->visible()) {
    double lowerChange = (range.lower - yAxesRanges[firstLeft].min) *
      yAxesRatios[firstRight];
    double upperChange = (range.upper - yAxesRanges[firstLeft].max) *
      yAxesRatios[firstRight];

    yAxesRanges[firstRight].min += lowerChange;
    yAxesRanges[firstRight].max += upperChange;
    axisRect->axis(QCPAxis::atRight)->setRange(yAxesRanges[firstRight].min,
      yAxesRanges[firstRight].max);

    if (axisRect->axisCount(QCPAxis::atLeft) == 2) {
      lowerChange = (range.lower - yAxesRanges[firstLeft].min) *
        yAxesRatios[secondLeft];
      upperChange = (range.upper - yAxesRanges[firstLeft].max) *
        yAxesRatios[secondLeft];

      yAxesRanges[secondLeft].min += lowerChange;
      yAxesRanges[secondLeft].max += upperChange;
      axisRect->axis(QCPAxis::atLeft, 1)->setRange(yAxesRanges[secondLeft].min,
        yAxesRanges[secondLeft].max);

      if (axisRect->axisCount(QCPAxis::atRight) == 2) {
        lowerChange = (range.lower - yAxesRanges[firstLeft].min) *
          yAxesRatios[secondRight];
        upperChange = (range.upper - yAxesRanges[firstLeft].max) *
          yAxesRatios[secondRight];

        yAxesRanges[secondRight].min += lowerChange;
        yAxesRanges[secondRight].max += upperChange;
        axisRect->axis(QCPAxis::atRight, 1)->setRange(yAxesRanges[secondRight].min,
          yAxesRanges[secondRight].max);
      }
    }

    yAxesRanges[firstLeft].min = range.lower;
    yAxesRanges[firstLeft].max = range.upper;
  }
}


void LogsDialog::addMaxAltitudeMarker(const coords_t & c, QCPGraph * graph) {
  // find max altitude
  int positionIndex = 0;
  double maxAlt = -100000;

  for(int i=0; i<c.x.count(); ++i) {
    double alt = c.y.at(i);
    if (alt > maxAlt) {
      maxAlt = alt;
      positionIndex = i;
      // qDebug() << "max alt: " << maxAlt << "@" << result;
    }
  }
  // qDebug() << "max alt: " << maxAlt << "@" << positionIndex;

  // add max altitude marker
  tracerMaxAlt = new QCPItemTracer(ui->customPlot);
  tracerMaxAlt->setGraph(graph);
  tracerMaxAlt->setInterpolating(true);
  tracerMaxAlt->setStyle(QCPItemTracer::tsSquare);
  tracerMaxAlt->setPen(QPen(Qt::blue));
  tracerMaxAlt->setBrush(Qt::NoBrush);
  tracerMaxAlt->setSize(7);
  tracerMaxAlt->setGraphKey(c.x.at(positionIndex));
  tracerMaxAlt->updatePosition();
}

void LogsDialog::countNumberOfThrows(const coords_t & c, QCPGraph * graph)
{
#if 0
  // find all launches
  // TODO
  double startTime = c.x.at(0);

  for(int i=0; i<c.x.count(); ++i) {
    double alt = c.y.at(i);
    double time = c.x.at(i);
  }
#endif
}

void LogsDialog::addCursor(QCPItemTracer ** cursor, QCPGraph * graph, const QColor & color) {
  QCPItemTracer * c = new QCPItemTracer(ui->customPlot);
  c->setGraph(graph);
  c->setInterpolating(false);
  c->setStyle(QCPItemTracer::tsCrosshair);
  QPen pen(color);
  pen.setStyle(Qt::DashLine);
  c->setPen(pen);
  c->setBrush(color);
  c->setSize(7);
  c->setVisible(false);
  *cursor = c;
}

void LogsDialog::addCursorLine(QCPItemStraightLine ** line, QCPGraph * graph, const QColor & color) {
  QCPItemStraightLine * l = new QCPItemStraightLine(ui->customPlot);
  l->point1->setParentAnchor(cursorA->position);
  l->point2->setParentAnchor(cursorB->position);
  QPen pen(color);
  pen.setStyle(Qt::DashLine);
  l->setPen(pen);
  l->setVisible(false);
  *line = l;
}
