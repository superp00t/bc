#include "bc/time/Time.hpp"
#include "bc/time/TimeConst.hpp"

#if defined(WHOA_SYSTEM_WIN)
#include <windows.h>
#endif

#if defined(WHOA_SYSTEM_MAC) || defined(WHOA_SYSTEM_LINUX)

#endif

namespace Blizzard {
namespace Time {

// Global variables

// Jan = [1],
// Feb = [2] and so on
static uint32_t s_monthDays[14] = {
    0x41F00000, // Invalid?
    0,
    31,
    59,
    90,
    120,
    151,
    182,
    212,
    243,
    273,
    304,
    334,
    365
};

// Functions

// Convert (input time) to unix seconds.
int32_t ToUnixTime(Timestamp timestamp) {
    // Can't return time prior to 1901
    // Return minimum value (1901)
    if (timestamp < -3094168447999999999ULL) {
        return -2147483648L;
    }

    // 32-bit unix sec time suffers from the Year 2038 problem
    // Return max time in case of overflow
    if (timestamp >= 1200798847000000000ULL) {
        return 2147483647L;
    }

    // Go back 30 years
    auto y1970 = timestamp + 946684800000000000ULL;
    // Convert nanoseconds to seconds
    return static_cast<uint32_t>(y1970 / TimeConst::TimestampsPerSecond);
}

Timestamp FromUnixTime(int32_t unixTime) {
    // Convert seconds to nanoseconds
    auto unixnano  = int64_t(unixTime) * TimeConst::TimestampsPerSecond;
    // Move forward 30 years
	auto y2k = unixnano - 946684800000000000ULL;

	return static_cast<Timestamp>(y2k);
}

// Win32 FILETIME to y2k
Timestamp FromWinFiletime(uint64_t winTime) {
    if (winTime < 33677863631452242ULL) {
        return -0x8000000000000000LL;
    }

    if (winTime >= 218145301729837567ULL) {
        return 0x7fffffffffffffffLL;
    }

    // 1601 (Gregorian) 100-nsec
    auto gregorian = static_cast<int64_t>(winTime);
    // Convert filetime from 1601 epoch to 2000 epoch.
    auto y2k = gregorian - TimeConst::WinFiletimeY2kDifference;
    // Convert 100-nsec intervals into nsec intervals
    return static_cast<Time::Timestamp>(y2k * 100LL);
}

uint64_t ToWinFiletime(Timestamp y2k) {
   return (y2k + TimeConst::WinFiletimeY2kDifference) / 100ULL;
}

int32_t GetTimeElapsed(uint32_t start, uint32_t end) {
    if (end < start) {
        return ~start + end;
    }
    return end - start;
}

Timestamp GetTimestamp() {
    return System_Time::Now();
}

uint64_t Nanoseconds() {
    return System_Time::Nanoseconds();
}

uint64_t Microseconds() {
    return System_Time::Microseconds();
}

uint32_t Milliseconds() {
    return System_Time::Milliseconds();
}

uint32_t Seconds() {
    return System_Time::Seconds();
}

Timestamp MakeTime(const TimeRec& date) {
 #if defined(WHOA_SYSTEM_WIN)
    // Win32 implementation
    FILETIME   fileTime   = {};
    SYSTEMTIME systemTime = {};

    systemTime.wYear   = static_cast<WORD>(date.year);
    systemTime.wMonth  = static_cast<WORD>(date.month);
    systemTime.wDay    = static_cast<WORD>(date.day);
    systemTime.wHour   = static_cast<WORD>(date.hour);
    systemTime.wMinute = static_cast<WORD>(date.min);
    systemTime.wSecond = static_cast<WORD>(date.sec);
    systemTime.wMilliseconds = 0;

    SystemTimeToFileTime(&systemTime, &fileTime);

    ULARGE_INTEGER ul = {};
    ul.HighPart = fileTime.dwHighDateTime;
    ul.LowPart  = fileTime.dwLowDateTime;

    auto timestamp = FromWinFiletime(ul.QuadPart);
    if (timestamp == -0x8000000000000000LL || timestamp == 0x7fffffffffffffffLL) {
        return timestamp;
    }

    timestamp += date.nsec;
#endif

#if defined(WHOA_SYSTEM_MAC) || defined(WHOA_SYSTEM_LINUX)
    // UNIX implementation
    struct tm t;

    t.tm_year = date.year  - 1900;
    t.tm_mon  = date.month - 1;
    t.tm_mday = date.day;
    t.tm_hour = date.hour;
    t.tm_min  = date.min;
    t.tm_sec  = date.sec;

    // Convert date into UNIX timestamp
    auto unixTime  = timegm(&t);
    auto timestamp = FromUnixTime(unixTime);
    if (timestamp == -2147483648L || timestamp == 2147483647L) {
        return timestamp;
    }

    timestamp += date.nsec;
#endif
    return timestamp;
}

void BreakTime(Timestamp timestamp, TimeRec& date) {
    auto nsec = timestamp % TimeConst::TimestampsPerSecond;

    if (nsec < 0) {
        nsec += TimeConst::TimestampsPerSecond;
    }

#if defined(WHOA_SYSTEM_WIN)
    // Win32 implementation
    ULARGE_INTEGER ul = {};
    ul.QuadPart = ToWinFiletime(timestamp);

    FILETIME fileTime = {};
    fileTime.dwHighDateTime = ul.HighPart;
    fileTime.dwLowDateTime  = ul.LowPart;
    SYSTEMTIME systemTime = {};
    ::FileTimeToSystemTime(&fileTime, &systemTime);

    date.day  = static_cast<uint32_t>(systemTime.wDay);
    date.hour = static_cast<uint32_t>(systemTime.wHour);
    date.min  = static_cast<uint32_t>(systemTime.wMinute);
    date.sec  = static_cast<uint32_t>(systemTime.wSecond);
    date.wday = static_cast<uint32_t>(systemTime.wDayOfWeek);
    date.year = static_cast<uint32_t>(systemTime.wYear);
    date.nsec = nsec;

    bool leapYear = (date.year % 400 == 0) || (date.year % 100 != 0 && ((systemTime.wYear & 3) == 0));

    auto yearDay = s_monthDays[date.month] + -1 + static_cast<uint32_t>(systemTime.wDay);
    date.yday = yearDay;
    if (leapYear && date.month > 2) {
        date.yday++;
    }
#endif

#if defined(WHOA_SYSTEM_MAC) || defined(WHOA_SYSTEM_LINUX)
    // UNIX implementation
    auto unixTime = ToUnixTime(timestamp);

    struct tm t;
    ::gmtime_r(unixTime, &t);

    date.year  = t.tm_year + 1900;
    date.month = t.tm_mon  + 1;
    date.day   = t.tm_mday;
    date.hour  = t.tm_hour;
    date.min   = t.tm_min;
    date.sec   = t.tm_sec;
    date.nsec  = nsec;
    date.wday  = t.tm_wday;
    date.yday  = t.tm_yday;
#endif
}

} // namespace Time
} // namespace Blizzard
