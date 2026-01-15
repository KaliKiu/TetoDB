// CommandDispatcher.h

#pragma once

#include "Common.h"
#include "Database.h"

extern Database* DB_INSTANCE;

Result ProcessCommand(string &line);
void ProcessDotCommand(string &line);