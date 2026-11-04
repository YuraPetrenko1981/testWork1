#pragma once

#include <QString>
#include <QIODevice>
#include <atomic>
#include <functional>
#include "logrecord.h"

LogColumnId logColumnIdFromHeader(const QString & header);
LogParseResult parseLogCsvText(const QString & text);
LogParseResult parseLogCsvDevice(QIODevice & device, qint64 fileSize,
                                 const std::function<void(int)> & progress,
                                 const std::atomic<bool> * cancelRequested);
