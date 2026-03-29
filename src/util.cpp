/*
 *  Copyright (C) 2016, Mike Walters <mike@flomp.net>
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

#include "util.h"
#include <clocale>

std::string formatSIValue(float value)
{
    std::map<int, std::string> prefixes = {
        {  9,   "G" },
        {  6,   "M" },
        {  3,   "k" },
        {  0,   ""  },
        { -3,   "m" },
        { -6,   "µ" },
        { -9,   "n" },
    };

    int power = 0;
    while (value < 1.0f && power > -9) {
        value *= 1e3;
        power -= 3;
    }
    while (value >= 1e3 && power < 9) {
        value *= 1e-3;
        power += 3;
    }
    std::stringstream ss;
    ss << value << prefixes[power];
    return ss.str();
}

std::string formatSIValueSigned(double value, const char *unit)
{
    static const struct { double threshold; double divisor; const char *suffix; } table[] = {
        { 1e9,  1e9,  "G" },
        { 1e6,  1e6,  "M" },
        { 1e3,  1e3,  "k" },
        { 0,    1.0,  ""  },
    };

    if (value == 0.0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "0%s", unit);
        return buf;
    }

    double av = (value < 0) ? -value : value;
    const char *suffix = "";
    double divisor = 1.0;

    for (auto &e : table) {
        if (av >= e.threshold) {
            divisor = e.divisor;
            suffix = e.suffix;
            break;
        }
    }

    double scaled = value / divisor;
    char buf[64];

    /* pick precision: show up to 4 significant digits */
    double as = (scaled < 0) ? -scaled : scaled;
    if (as >= 100.0)
        snprintf(buf, sizeof(buf), "%.1f%s%s", scaled, suffix, unit);
    else if (as >= 10.0)
        snprintf(buf, sizeof(buf), "%.2f%s%s", scaled, suffix, unit);
    else
        snprintf(buf, sizeof(buf), "%.3f%s%s", scaled, suffix, unit);

    return buf;
}

bool parseSIValue(const std::string &str, double &result)
{
    if (str.empty())
        return false;

    char *prev = setlocale(LC_NUMERIC, nullptr);
    std::string savedLocale(prev ? prev : "");
    setlocale(LC_NUMERIC, "C");

    char *end = nullptr;
    double val = strtod(str.c_str(), &end);

    if (!savedLocale.empty())
        setlocale(LC_NUMERIC, savedLocale.c_str());

    if (end == str.c_str())
        return false;

    while (*end == ' ')
        end++;

    switch (*end) {
    case 'G': case 'g': val *= 1e9;  break;
    case 'M':           val *= 1e6;  break;
    case 'K': case 'k': val *= 1e3;  break;
    case 'm':           val *= 1e-3; break;
    case 'u':           val *= 1e-6; break;
    case 'n':           val *= 1e-9; break;
    case '\0':          break;
    default:            break; /* ignore trailing unit text (Hz, Bd, etc.) */
    }

    result = val;
    return true;
}

template<> const char* getFileNameFilter<std::complex<float>>() { return "complex<float> file (*.fc32)"; };
template<> const char* getFileNameFilter<float>() { return "float file (*.f32)"; };
