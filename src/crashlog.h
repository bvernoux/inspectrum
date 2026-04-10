/*
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  This file is part of inspectrum.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>

/*
 * Lightweight crash and diagnostic logger.
 *
 * - init() collects OS / build info and opens (or creates) the log file.
 * - log() appends a timestamped entry.
 * - installCrashHandlers() registers platform-specific handlers that
 *   write a final entry before the process dies.
 *
 * The log file is placed next to the executable:
 *   <exe_dir>/inspectrum_crash.log
 *
 * All functions are safe to call from any thread.  The file writes are
 * serialised with a simple mutex so entries don't interleave.
 */
namespace CrashLog
{

enum Severity {
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATAL
};

/* Must be called once, early in main(). */
void init(const char *appName, const char *appVersion);

/* Append a line to the log file. */
void log(Severity severity, const char *fmt, ...);

/* Register OS-level crash handlers (SEH on Windows, signals on Unix). */
void installCrashHandlers();

/* Return the full path of the log file (available after init). */
const std::string &logFilePath();

} // namespace CrashLog
