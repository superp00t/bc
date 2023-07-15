#include "bc/file/File.hpp"
#include "test/Test.hpp"

#if defined(WHOA_SYSTEM_WIN)
#include <windows.h>

TEST_CASE("Blizzard::File::Open", "[file]") {
    SECTION("Opens and closes a system file successfully") {
        const char* filename = "C:\\Windows\\System32\\cmd.exe";

        Blizzard::File::StreamRecord* stream = nullptr;

        bool ok = Blizzard::File::Open(filename, BC_FILE_OPEN_READ, stream);

        REQUIRE(ok == true);
        REQUIRE(stream != nullptr);

        bool closed = Blizzard::File::Close(stream);
        REQUIRE(closed == true);
    }

    SECTION("Can open system file, read a small segment of data successfully, then close successfullly") {
        const char* filename = "C:\\Windows\\System32\\cmd.exe";

        Blizzard::File::StreamRecord* stream = nullptr;

        bool ok = Blizzard::File::Open(filename, BC_FILE_OPEN_READ, stream);

        REQUIRE(ok == true);
        REQUIRE(stream != nullptr);

        uint8_t bytes[8] = {0};
        size_t bytesRead = 0;

        bool readok = Blizzard::File::Read(stream, bytes, 8, &bytesRead, 0);
        REQUIRE(readok);
        REQUIRE(bytesRead == 8);

        REQUIRE(bytes[0] == 0x4D);
        REQUIRE(bytes[1] == 0x5A);

        bool closed = Blizzard::File::Close(stream);
        REQUIRE(closed == true);
    }

    SECTION("Can open system file, read a small segment of data successfully, then close successfullly") {
        const char* filename = "C:\\Windows\\System32\\cmd.exe";

        Blizzard::File::StreamRecord* stream = nullptr;

        bool ok = Blizzard::File::Open(filename, BC_FILE_OPEN_READ, stream);

        REQUIRE(ok == true);
        REQUIRE(stream != nullptr);

        uint8_t bytes[8] = {0};
        size_t bytesRead = 0;

        bool readok = Blizzard::File::Read(stream, bytes, 8, &bytesRead, 0);
        REQUIRE(readok);
        REQUIRE(bytesRead == 8);

        REQUIRE(bytes[0] == 0x4D);
        REQUIRE(bytes[1] == 0x5A);

        bool closed = Blizzard::File::Close(stream);
        REQUIRE(closed == true);
    }

    SECTION("Can open a test file in your local config folder")
}

#endif
