#include "bc/Time.hpp"
#include "test/Test.hpp"

TEST_CASE("Blizzard::Time::FromUnixTime", "[time]") {
    SECTION("create zero timestamp from Y2K unix date") {
        auto stamp = Blizzard::Time::FromUnixTime(946684800);
        REQUIRE(stamp == 0);
    }
}
