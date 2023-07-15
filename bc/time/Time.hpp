#ifndef BC_TIME_TIME_HPP
#define BC_TIME_TIME_HPP

#include "bc/time/Types.hpp"
#include "bc/time/system/System_Time.hpp"

#include <cstdint>

#if defined(WHOA_SYSTEM_WIN)
#include <windows.h>
#endif

namespace Blizzard {
namespace Time {

int32_t   ToUnixTime(Timestamp timestamp);

Timestamp FromUnixTime(int32_t unix);

#if defined(WHOA_SYSTEM_WIN)
// Win32 FILETIME to y2k
Timestamp FromWinFiletime(FILETIME* ft);
void      ToWinFiletime(Timestamp timestamp, FILETIME* ft);
#endif

Timestamp GetTimestamp();

int32_t   GetTimeElapsed(uint32_t start, uint32_t end);

Timestamp MakeTime(TimeRec& date);

void      BreakTime(Timestamp timestamp, TimeRec& date);

uint64_t  Nanoseconds();

uint64_t  Microseconds();

uint32_t  Milliseconds();

uint32_t  Seconds();

} // namespace Time
} // namespace Blizzard

#endif
