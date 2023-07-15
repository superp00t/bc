#include "bc/Time.hpp"
#include "test/Test.hpp"

TEST_CASE("Blizzard::Time::FromUnixTime", "[thread]") {
    SECTION("constructs value and stores it in available TLS slot") {
        REQUIRE(*reinterpret_cast<int32_t*>(ptr) == value);
    }
}
