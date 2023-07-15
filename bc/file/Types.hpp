#ifndef BC_FILE_TYPES_HPP
#define BC_FILE_TYPES_HPP

#include "bc/file/system/Defines.hpp"
#include "bc/time/Time.hpp"

#if defined(WHOA_SYSTEM_WIN)
#include <windows.h>
#endif

#include <cstddef>

namespace Blizzard {
namespace File {

// Types

// Used by SetCacheMode
enum Mode {
    setperms = 4,
    settimes = 16,
    nocache  = 64
};

class FileInfo {
    public:
        Time::TimeRec*  time;
        // The device ID storing this file.
        uint32_t        device;
        // Tell if a file is a directory
        // read-only, hidden
        // See file/Defines.hpp for more
        uint32_t        attributes;
        // Size in bytes
        uint64_t        size;
        // Note that these are Y2K time, not Unix
        Time::Timestamp accessTime;
        Time::Timestamp modificationTime;
        Time::Timestamp attributeModificationTime;
};

class ProcessDirParms {
    public:
        const char* root            = nullptr;
        const char* item            = nullptr;
        bool        itemIsDirectory = false;
        void*       param           = nullptr;
};

typedef bool (*ProcessDirCallback)(const ProcessDirParms&);

class StreamRecord {
    public:
        static constexpr size_t s_padPath = 80;

#if defined(WHOA_SYSTEM_WIN)
        HANDLE      filehandle;
#endif
#if defined(WHOA_SYSTEM_MAC) || defined(WHOA_SYSTEM_LINUX)
        // File descriptor
        int32_t     filefd;
#endif
        // Open flags
        uint32_t    flags;
        // Determines whether info object is initialized
        bool        hasInfo;
        FileInfo    info;
        // The path of the opened file.
        char        path[s_padPath];

};

} // namespace File
} // namespace Blizzard

#endif
