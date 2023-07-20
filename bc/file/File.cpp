#include "bc/file/File.hpp"
#include "bc/file/Filesystem.hpp"
#include "bc/file/system/Stacked.hpp"

#include <cstdio>
#include <cinttypes>

namespace Blizzard {
namespace File {

// Functions

void SetLastError(int32_t errorcode) {
    // TODO
}

void AddToLastErrorStack(int32_t errorcode, const char* msg, int32_t a3) {
    // TODO: use proper logging

    char empty[] = "";

    if (!msg) {
        msg = empty;
    }

    fprintf(stderr, "[bc/file] %" PRIi32 ": '%s'\n", errorcode, msg);
}

// In BlizzardCore parlance, a file only exists if its path points to a regular file. (not a directory)
// i.e. directories are not considered to be file objects.
bool Exists(const char* path) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    FileInfo info = {};

    System_File::Stacked::FileParms parms = {};
    parms.filename = path;
    parms.info     = &info;

    auto status = manager->Do(Filesystem::Call::Exists, &parms);

    return status;
}

// Alias of Exists.
bool IsFile(const char* path) {
    return Exists(path);
}

// Calls Exists internally, only checking whether it has the directory attribute instead of normal attribute
bool IsDirectory(const char* path) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    FileInfo info = {};

    System_File::Stacked::FileParms parms = {};
    parms.filename = path;
    parms.info     = &info;

    auto status = manager->Do(Filesystem::Call::Exists, &parms);

    return info.attributes & BC_FILE_ATTRIBUTE_DIRECTORY;
}

// Delete a file.
bool Delete(const char* path) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};
    parms.filename = path;

    auto status = manager->Do(Filesystem::Call::Delete, &parms);
    return status;
}

// Get file information about path, writing information to the passed info pointer.
bool GetFileInfo(const char* path, FileInfo* info) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};
    parms.filename = path;
    parms.info     = info;

    auto status = manager->Do(Filesystem::Call::GetFileInfo, &parms);

    return status;
}

// Get file information from a StreamRecord, writing information to the passed info pointer.
bool GetFileInfo(StreamRecord* stream, FileInfo* info) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};
    parms.stream = stream;
    parms.info   = info;

    return manager->Do(Filesystem::Call::GetFileInfo, &parms);
}

bool GetWorkingDirectory(char* path, size_t capacity) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};
    parms.directory     = path;
    parms.directorySize = capacity;

    return manager->Do(Filesystem::Call::GetWorkingDirectory, &parms);
}

// Get file information from a stream record, returning a file info pointer owned by StreamRecord
// The FileInfo ptr returned is invalidated after a call to File::Close(stream)
FileInfo* GetFileInfo(StreamRecord* stream) {
    static FileInfo s_noinfo = {};

    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return &s_noinfo;
    }

    System_File::Stacked::FileParms parms = {};
    parms.stream = stream;
    parms.info   = nullptr;

    auto status = manager->Do(Filesystem::Call::GetFileInfo, &parms);

    if (status) {
        return parms.info;
    }

    return &s_noinfo;
}

bool MakeAbsolutePath(const char* rel, char* result, int32_t capacity, bool unkflag) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};

    parms.filename      = rel;
    parms.flag          = unkflag;
    parms.directory     = result;
    parms.directorySize = capacity;

    return manager->Do(Filesystem::Call::MakeAbsolutePath, &parms);
}

// Open a filename according to flags (see Defines.hpp)
// if successful, will return true and referenced file object will be set
bool Open(const char* filename, uint32_t flags, StreamRecord*& file) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};

    parms.filename = filename;
    parms.flag     = flags;

    auto status = manager->Do(Filesystem::Call::Open, &parms);

    if (status) {
        file = parms.stream;
    }

    return status;
}

bool SetAttributes(const char* filename, uint32_t attributes) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};
    parms.filename = filename;
    parms.flag     = static_cast<uint32_t>(attributes);
    parms.mode     = File::Mode::setperms;

    return manager->Do(Filesystem::Call::SetAttributes, &parms);
}

bool Close(StreamRecord* stream) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};
    parms.stream = stream;

    auto status = manager->Do(Filesystem::Call::Close, &parms);
    return status;
}

bool Copy(const char* source, const char* destination, bool overwrite) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};
    parms.filename    = source;
    parms.destination = destination;

    return manager->Do(Filesystem::Call::Copy, &parms);
}

bool ProcessDirFast(const char* path, void* param, ProcessDirCallback callback, bool unkflag) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};

    parms.filename = path;
    parms.param    = param;
    parms.flag     = unkflag;
    parms.callback = callback;

    return manager->Do(Filesystem::Call::ProcessDirFast, &parms);
}

bool Read(StreamRecord* file, void* buffer, size_t bytesToRead, size_t* bytesRead, int32_t flag) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};

    parms.stream   = file;
    parms.param    = buffer;
    parms.size     = bytesToRead;
    parms.flag     = flag;

    auto status = manager->Do(Filesystem::Call::Read, &parms);

    if (bytesRead) {
        *bytesRead = static_cast<size_t>(parms.size);
    }

    return status;
}

bool Write(StreamRecord* file, void* buffer, size_t bytesToWrite, size_t* bytesWritten, int32_t flag) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};

    parms.stream = file;
    parms.param  = buffer;
    parms.size   = bytesToWrite;
    parms.flag   = flag;

    auto status = manager->Do(Filesystem::Call::Write, &parms);

    if (bytesWritten) {
        *bytesWritten = static_cast<size_t>(parms.size);
    }

    return status;
}

bool SetPos(StreamRecord* file, int64_t pos, int32_t whence) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};

    parms.stream   = file;
    parms.position = pos;
    parms.whence   = whence;

    return manager->Do(Filesystem::Call::SetPos, &parms);
}

bool GetPos(StreamRecord* file, int64_t& pos) {
    auto manager = System_File::Stacked::Manager();
    if (!manager) {
        return false;
    }

    System_File::Stacked::FileParms parms = {};

    parms.stream   = file;
    parms.position = pos;

    auto status = manager->Do(Filesystem::Call::GetPos, &parms);
    if (status) {
        pos = parms.position;
    }
    return status;
}

} // namespace File
} // namespace Blizzard
