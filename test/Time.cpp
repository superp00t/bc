#include "bc/Time.hpp"
#include "test/Test.hpp"

TEST_CASE("Blizzard::Time::FromUnixTime", "[time]") {
    SECTION("create correct timestamp from unix time") {
        auto stamp = Blizzard::Time::FromUnixTime(946707780);
        REQUIRE(stamp == 0);
    }
}
