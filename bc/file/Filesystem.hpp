#ifndef BC_FILE_FILESYSTEM_HPP
#define BC_FILE_FILESYSTEM_HPP

#include "bc/file/system/Types.hpp"
#include <cstdint>

namespace Blizzard {
namespace File {

// Filesystem describes the layout of a virtual filesystem.
// It is linked to other Filesystems in a list structure.
class Filesystem {
    public:
        enum class Call : uint32_t {
            SetWorkingDirectory,
            Close,
            Create,
            GetWorkingDirectory,
            ProcessDirFast,
            Exists,
            Flush,
            GetFileInfo,
            GetFreeSpace,
            GetPos,
            GetRootChars,
            IsAbsolutePath,
            IsReadOnly,
            MakeAbsolutePath,
            CreateDirectory,
            Move,
            Copy,
            Open,
            Read,
            ReadP,
            RemoveDirectory,
            SetCacheMode,
            SetEOF,
            SetAttributes,
            SetPos,
            Delete,
            Write,
            WriteP,
            Shutdown,
            EndOfCalls
        };

        // I hate C++ enums
        static constexpr size_t s_numCalls = static_cast<size_t>(Call::EndOfCalls);

        // One signature for all filesystem calls
        typedef bool (*CallFunc)(Filesystem*, System_File::Stacked::FileParms*);

        // The source filesystem (where it was copied from)
        // You can tell if base == this, that it is the original stack and not part of the filesystem manager
        Filesystem* base;
        // The next filesystem (towards lower filesystems)
        Filesystem* next;
        CallFunc    funcs[s_numCalls];

        bool Do(Call call, System_File::Stacked::FileParms* parms);
};

} // namespace File
} // namespace Blizzard

#endif
