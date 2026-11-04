#pragma once

#include <QString>
#include "logrecord.h"

LogColumnId logColumnIdFromHeader(const QString & header);
LogParseResult parseLogCsvText(const QString & text);
