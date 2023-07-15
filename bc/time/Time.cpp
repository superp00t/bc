#include "bc/time/Time.hpp"

namespace Blizzard {
namespace Time {

// Global variables

// amount of win32 filetime units in a second
constexpr uint64_t win32filetimeUnitsPerSec   = (BC_NSEC_PER_SEC / 100ULL);
// the FILETIME value needed to move from 1601 epoch to the Year 2000 epoch that Blizzard prefers
constexpr uint64_t win32filetimeY2kDifference = 125911584000000000ULL;

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
    return static_cast<uint32_t>(y1970 / BC_NSEC_PER_SEC);
}

Timestamp FromUnixTime(int32_t unix) {
    // Convert seconds to nanoseconds
    auto unixnano  = int64_t(unix) * BC_NSEC_PER_SEC;
    // Move forward 30 years
	auto y2k = unixnano - 946684800000000000ULL;

	return static_cast<Timestamp>(y2k);
}


#if defined(WHOA_SYSTEM_WIN)

// Win32 FILETIME to y2k
Timestamp FromWinFiletime(FILETIME* ft) {
    // 1601 (Gregorian) 100-nsec
    auto gregorian = (static_cast<uint64_t>(ft->dwHighDateTime) << 32ULL) | static_cast<uint64_t>(ft->dwLowDateTime);
    // Convert filetime from 1601 epoch to 2000 epoch.
    auto y2k = gregorian - win32filetimeY2kDifference;
    // Convert 100-nsec intervals into nsec intervals
    return static_cast<Time::Timestamp>(y2k * 100ULL);
}

void ToWinFiletime(Timestamp y2k, FILETIME* ft) {
    auto gregorian = (y2k + win32filetimeY2kDifference) / 100ULL;

    ft->dwLowDateTime  = static_cast<DWORD>(gregorian);
    ft->dwHighDateTime = static_cast<DWORD>(gregorian >> 32ULL);
}

#endif

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

Timestamp MakeTime(TimeRec& date) {
    // TODO: implement
    return 0;
}

void BreakTime(Timestamp timestamp, TimeRec& date) {
    // TODO: implement
}

} // namespace Time
} // namespace Blizzard
