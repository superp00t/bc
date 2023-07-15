#ifndef BC_TIME_TYPES_HPP
#define BC_TIME_TYPES_HPP

#include <cstdint>

#define BC_NSEC_PER_SEC 1000000000ULL

namespace Blizzard {
namespace Time {

// Timestamp - nanoseconds starting from 0 == January 1 2000 00:00:00 GMT.
typedef int64_t Timestamp;

class TimeRec {
    public:
        int32_t  year;
        int32_t  month;
        int32_t  day;
        int32_t  hour;
        int32_t  min;
        int32_t  sec;
        uint32_t nsec;
};

} // namespace Time
} // namespace Blizzard

#endif
