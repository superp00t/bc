#ifndef BC_FILE_SYSTEM_TYPES_HPP
#define BC_FILE_SYSTEM_TYPES_HPP

#include "bc/Debug.hpp"
#include "bc/file/Types.hpp"

#include <cstdint>

namespace Blizzard {

namespace System_File {

class FileError {
    public:
        // Error codes can be any value, even negative.
        // Look for common error codes in Defines.hpp
        // on POSIX, errorcode could be a POSIX error offset +100.
        int32_t                  errorcode;
        Debug::ErrorStackRecord* stack;
};

namespace Stacked {

// FileParms stores parameters useful to all filesystem interactions
// so all fs calls can use one signature
class FileParms {
    public:
        // The byte offset to the filesystem action func of this call.
        uintptr_t                 offset;
        // The "source" file object
        const char*               filename;
        // The "destination" file object, or just a user parameter.
        union {
            void*                 param;
            const char*           destination;
        };
        // Supplies the handle/fd of a file
        File::StreamRecord*       stream;
        // Flag or boolean field.
        uint32_t                  flag;
        // Stores additional status bits.
        uint32_t                  mode;
        File::FileInfo*           info;
        // Beginning and end used when querying slices: such as GetRootChars
        union {
            // The specified position of the interaction
            int64_t               position;
            int64_t               beginning;
        };
        union {
            int32_t               whence;
            int64_t               end;
        };
        union {
            // size_t is not guaranteed to be 64-bit
            // size64 is included here as well to allow interactions on files > 4 GB in size.
            uint64_t              size64;
            size_t                size;
        };
        File::ProcessDirCallback  callback;
        char*                     directory;
        size_t                    directorySize;
};

} // namespace Stacked

} // namespace System_File
} // namespace Blizzard

#endif
