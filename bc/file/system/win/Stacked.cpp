#include "bc/file/Defines.hpp"
#include "bc/file/File.hpp"
#include "bc/file/Path.hpp"
#include "bc/file/system/Stacked.hpp"
#include "bc/file/system/win/Utils.hpp"
#include "bc/Debug.hpp"
#include "bc/Memory.hpp"
#include "bc/String.hpp"

#include <algorithm>

/********************************
* Begin Win32 Stacked functions *
*********************************/

namespace Blizzard {
namespace System_File {
namespace Stacked {

bool SetWorkingDirectory(FileParms* parms) {
    BLIZZARD_ASSERT(parms->filename);
    return ::SetCurrentDirectory(parms->filename) != 0;
}

bool Close(FileParms* parms) {
    auto file = parms->stream;
    BLIZZARD_ASSERT(file != nullptr);

    ::CloseHandle(file->filehandle);
    Memory::Free(file);

    return true;
}

bool GetWorkingDirectory(FileParms* parms) {
    if (!parms->directory || !parms->directorySize) {
        // System_File::FileError(8)
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    auto size      = static_cast<DWORD>(parms->directorySize);
    auto directory = static_cast<LPSTR>(parms->directory);

    ::GetCurrentDirectory(size, directory);

    return true;
}

bool ProcessDirFast(FileParms* parms) {
    // Get parameters
    auto unkflag   = parms->flag;
    auto directory = parms->filename;
    auto callback  = parms->callback;
    auto param     = parms->param;

    // Set up walk parameters
    File::ProcessDirParms processDirParms;
    processDirParms.root  = directory;
    processDirParms.param = param;

    constexpr uint32_t formatSize = BC_FILE_MAX_PATH * 2;
    char formatted[formatSize] = "";

    // Tack on a backslash as well as an asterisk.
    String::Format(formatted, formatSize, "%s\\*", processDirParms.root);

    // Convert potentially large path to universal format
    BC_FILE_PATH(path);
    if (!File::Path::MakeNativePath(formatted, path, formatSize)) {
        return false;
    }

    //
    WIN32_FIND_DATA findData;

    auto hFindFile = ::FindFirstFile(formatted, &findData);

    if (hFindFile == INVALID_HANDLE_VALUE) {
        BC_FILE_SET_ERROR_MSG(BC_FILE_ERROR_INVALID_ARGUMENT, "Win32 ProcessDirFast - %s", directory);
        return false;
    }

    BC_FILE_PATH(currentPath);
    processDirParms.item = currentPath;

    while (true) {
        // Ignore .. and .
        if (findData.cFileName[0] != '.' || findData.cFileName[1] && (findData.cFileName[1] != '.' || findData.cFileName[2])) {
            String::Copy(currentPath, findData.cFileName, BC_FILE_MAX_PATH);
            processDirParms.itemIsDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            if (!callback(processDirParms)) {
                ::FindClose(hFindFile);
                return true;
            }

            if (!::FindNextFile(hFindFile, &findData)) {
                break;
            }
        }
    }

    ::FindClose(hFindFile);
    return false;
}

bool Exists(FileParms* parms) {
    auto filepath = parms->filename;
    if (!filepath) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    File::Path::QuickNative filepathNative(filepath);

    auto dwFileAttributes = ::GetFileAttributes(static_cast<LPCSTR>(filepathNative.Str()));

    if (dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
        parms->info->attributes = 0;
        return false;
    }

    uint32_t fileAttributes = winfileutil::AttributesToBC(dwFileAttributes);
    parms->info->attributes = fileAttributes;

    return fileAttributes & BC_FILE_ATTRIBUTE_NORMAL;
}

bool Flush(FileParms* parms) {
    auto file = parms->stream;

    if (file == nullptr || file->filehandle == INVALID_HANDLE_VALUE ) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    auto status = ::FlushFileBuffers(file->filehandle);
    return status != 0;
}

bool GetFileInfo(FileParms* parms) {
    auto  file     = parms->stream;
    auto  filepath = parms->filename;
    auto  info     = parms->info;

    if (filepath == nullptr && file == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    // StreamRecord* file may be used to supply storage of file info. (StreamRecord*->info)
    if (file) {
        BLIZZARD_ASSERT(file->filehandle != INVALID_HANDLE_VALUE);
        if (info == nullptr) {
            // telling this function to save file info into StreamRecord
            info = &file->info;
        }
    }

    if (filepath) {
        // Fetch info based on filepath.
        static File::FileInfo s_noinfo = {};
        if (info == nullptr) {
            info = &s_noinfo;
        }

        File::Path::QuickNative filepathNative(filepath);

        WIN32_FILE_ATTRIBUTE_DATA fileAttributeData = {};

        // Read attributes
        if (!::GetFileAttributesExA(filepathNative.Str(), GetFileExInfoStandard, &fileAttributeData)) {
            BC_FILE_SET_ERROR_MSG(BC_FILE_ERROR_INVALID_HANDLE, "Win32 GetFileInfo - GetFileAttributesExA failed");
            return false;
        }

        return winfileutil::AttributeFileInfoToBC(&fileAttributeData, info);
    }

    // Fetch info using opened file handle
    auto filehandle = file->filehandle;

    BY_HANDLE_FILE_INFORMATION byHandleInfo;
    if (!GetFileInformationByHandle(filehandle, &byHandleInfo)) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_HANDLE);
        return false;
    }

    file->hasInfo = true;

    return winfileutil::HandleFileInfoToBC(&byHandleInfo, info);
}

bool GetFreeSpace(FileParms* parms) {
    auto filename = parms->filename;

    if (filename == nullptr || *filename == '\0') {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    auto len = String::Length(filename) + 2;
    BC_FILE_PATH(systemPath);
    // UNC convert
    File::Path::MakeNativePath(filename, systemPath, BC_FILE_MAX_PATH);

    // GetDiskFreeSpaceExA will fail without a trailing backslash
    File::Path::ForceTrailingSeparator(systemPath, BC_FILE_MAX_PATH, BC_FILE_SYSTEM_PATH_SEPARATOR);

    ULARGE_INTEGER freeBytesAvailableToCaller = {};
    ULARGE_INTEGER totalNumberOfBytes         = {};
    ULARGE_INTEGER totalNumberOfFreeBytes     = {};

    if (!GetDiskFreeSpaceExA(systemPath, &freeBytesAvailableToCaller, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    parms->size64 = static_cast<uint64_t>(freeBytesAvailableToCaller.QuadPart);
    return true;
}

bool GetPos(FileParms* parms) {
    auto file = parms->stream;

    if (file == nullptr || file->filehandle == INVALID_HANDLE_VALUE) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    LONG  high = 0;
    DWORD low  = ::SetFilePointer(file->filehandle, 0, &high, FILE_CURRENT);
    if (low == -1 && GetLastError()) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_HANDLE);
        return false;
    }

    ULARGE_INTEGER ul = {};
    ul.LowPart  = low;
    ul.HighPart = high;

    parms->position = ul.QuadPart;
    return true;
}

bool GetRootChars(FileParms* parms) {
    char cVar1 = '\0';
    int32_t counter = 0;
    int32_t size = 0;
    char *buffer = nullptr;
    const char *path = nullptr;
    char unixPathFast[256] = {0};
    const char *end = nullptr;

    path = parms->filename;
    counter = String::Length(path);
    size = counter + 1;
    if (size > 256) {
        buffer = reinterpret_cast<char *>(Memory::Allocate(size));
    } else {
        buffer = unixPathFast;
    }

    // Invalid without error
    if (size < 0x2) {
        parms->end = 0;
        parms->beginning = 0;
    } else {
        File::Path::MakeUnivPath(path, buffer, size);
        path = buffer + 1;
        // Get DOS canonical path
        if (buffer[1] == ':') {
            cVar1 = buffer[2];
            parms->beginning = 0;
            parms->end = (cVar1 == '/') + 2;
        } else {
            // Get UNC path
            if ((*buffer == '/') && (buffer[1] == '/')) {
                counter = 0;
                do {
                    path = strchr(path + 1, '$');
                    if (path == nullptr) {
                        parms->end = size;
                        parms->beginning = 0;
                        if (buffer != unixPathFast) {
                            Memory::Free(buffer);
                        }
                        return true;
                    }
                    counter = counter + 1;
                } while (counter < 2);
                parms->end = intptr_t(path) + (1 - (intptr_t)buffer);
            } else {
                parms->end = 0;
            }
            parms->beginning = 0;
        }
    }

    if (buffer != unixPathFast) {
        Memory::Free(buffer);
    }

    return true;
}

bool IsAbsolutePath(FileParms* parms) {
    auto path = parms->filename;

    if (!path) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    auto first = path[0];

    if (
        // UNC
        (
            ((first == '\\') || (first == '/')) &&
            ((path[1] == '\\' || (path[1] == '/')))
        ) &&
            (path[2] != '\0')) {
            return true;
    }

    // DOS canonical
    if (isalpha(first) && (path[1] == ':')) {
        if (((path[2] == '\\') || (path[2] == '/')) && (path[3] != '\0')) {
            return true;
        }
    }

    return false;
}

bool IsReadOnly(FileParms* parms) {
    auto filepath = parms->filename;
    if (!filepath) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    File::Path::QuickNative filepathNative(filepath);

    auto dwFileAttributes = ::GetFileAttributes(filepathNative.Str());

    if (dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    return (dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
}

bool MakeAbsolutePath(FileParms* parms) {
    BC_FILE_PATH(full);

    auto path = parms->filename;

    if (path == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    ::_fullpath(full, path, BC_FILE_MAX_PATH);

    File::Path::ForceTrailingSeparator(full, BC_FILE_MAX_PATH, BC_FILE_SYSTEM_PATH_SEPARATOR);

    String::Copy(parms->directory, full, parms->directorySize);

    return true;
}

bool CreateDirectory(FileParms* parms) {
    auto path = parms->filename;

    char temp[300] = {0};
    auto p = path;

    size_t count = 0;

    while (*p != '\0') {
        if (*p == '\\' || *p == '/') {
            count++;
            int32_t len = p - path;
            if (len == 0) {
                len = 1;
            }
            String::Copy(temp, path, len);
            temp[len] = '\0';

            if (GetFileAttributes(temp) == INVALID_FILE_ATTRIBUTES) {
                if (!::CreateDirectory(temp, nullptr)) {
                    if (GetLastError() != ERROR_ALREADY_EXISTS) {
                        return false;
                    }
                }
            }
        }
        p++;
    }

    if (count == 0) {
        // No directories were made because there are no path separators.
        // Make only one directory:

        String::Copy(temp, path, 300);

        if (::GetFileAttributes(temp) == INVALID_FILE_ATTRIBUTES) {
            if (!::CreateDirectory(temp, nullptr)) {
                if (::GetLastError() != ERROR_ALREADY_EXISTS) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool Move(FileParms* parms) {
    auto source      = parms->filename;
    auto destination = parms->destination;

    File::Path::QuickNative sourceNative(source);
    File::Path::QuickNative destinationNative(destination);

    BOOL ok = ::MoveFile(sourceNative.Str(), destinationNative.Str());

    return ok != 0;
}

bool Copy(FileParms* parms) {
    auto source      = parms->filename;
    auto destination = parms->destination;

    auto overwrite = parms->flag;

    // file pointers
    File::StreamRecord* st_source      = nullptr;
    File::StreamRecord* st_destination = nullptr;

    // Flags for working with src and dst files
    auto flag_source      = BC_FILE_OPEN_LOCK | BC_FILE_OPEN_READ | BC_FILE_OPEN_MUST_EXIST;
    auto flag_destination = BC_FILE_OPEN_LOCK | BC_FILE_OPEN_WRITE | BC_FILE_OPEN_CREATE;
    if (!overwrite) {
        // User commands that we cannot overwrite. Fail if file already exists
        flag_destination |= BC_FILE_OPEN_MUST_NOT_EXIST;
    } else {
        // We are supposed to overwrite, so truncate the file to 0 bytes.
        flag_destination |= BC_FILE_OPEN_TRUNCATE;
    }

    // Open source file to be copied
    if (!File::Open(source, flag_source, st_source)) {
        // FileError(2)
        BC_FILE_SET_ERROR(BC_FILE_ERROR_FILE_NOT_FOUND);
        return false;
    }

    // Open (or create if it doesn't exist) destination file
    if (!File::Open(destination, flag_destination, st_destination)) {
        File::Close(st_source);
        // FileError(4)
        BC_FILE_SET_ERROR(BC_FILE_ERROR_BUSY);
        return false;
    }

    // Determine buffer copy size
    auto sz_source = File::GetFileInfo(st_source)->size;

    // copybuffer size upper limit is BC_FILE_SYSTEM_COPYBUFFER_SIZE
    size_t sz_copybuffer = std::min(sz_source, uint64_t(BC_FILE_SYSTEM_COPYBUFFER_SIZE));
    auto   u8_copybuffer = reinterpret_cast<uint8_t*>(Memory::Allocate(sz_copybuffer));

    // Loop through the source file, reading segments into copybuffer
    for (uint64_t index = 0; index < sz_source; index += sz_copybuffer) {
        // How many bytes to read
        size_t sz_bytesToRead = sz_source - std::min(index+sz_copybuffer, sz_source);
        size_t sz_bytesRead = 0;
        size_t sz_bytesWritten = 0;
        // Read data segment into copybuffer
        auto status = File::Read(st_source, u8_copybuffer, sz_bytesToRead, &sz_bytesRead, 0);
        if (status) {
            // Write copied segment to destination file
            status = File::Write(st_destination, u8_copybuffer, sz_bytesRead, &sz_bytesWritten, 0);
        }

        if (!status) {
            // either read or write failed: cleanup copy buffer
            Memory::Free(u8_copybuffer);
            // close files
            File::Close(st_source);
            File::Close(st_destination);
            // Delete malformed file
            File::Delete(destination);
            // return bad status, but without creating a file error.
            return false;
        }
    }

    Memory::Free(u8_copybuffer);
    // close files
    File::Close(st_source);
    File::Close(st_destination);

    // Success!
    return true;
}

bool Open(FileParms* parms) {
    // Path convert
    auto path = parms->filename;
    File::Path::QuickNative pathNative(path);

    // Open file HANDLE
    auto   flags   =  parms->flag;
    bool   nocache  = parms->mode & File::Mode::nocache;
    HANDLE handle   = winfileutil::Open(pathNative.Str(), flags, nocache);

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = 0;
        if (err = GetLastError()) {
            BC_FILE_SET_ERROR_MSG(BC_FILE_ERROR_GENERIC_FAILURE, "Win32 Open %s", parms->filename);
        }
        return false;
    }

    // Successfully opened file handle. Allocate StreamRecord + path str at the end.
    auto recordSize = (sizeof(File::StreamRecord) - File::StreamRecord::s_padPath) + (1 + pathNative.Size());
    auto fileData   = Memory::Allocate(recordSize);
    // Memory could not be allocated
    if (fileData == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_OOM);
        return false;
    }
    // Clear extra data
    String::MemFill(fileData, recordSize, 0);

    // Populate fields
    auto file = reinterpret_cast<File::StreamRecord*>(fileData);
    file->flags      = flags;
    file->filehandle = handle;
    String::Copy(file->path, path, pathNative.Size());
    File::GetFileInfo(file);

    parms->stream = file;

    return true;
}

bool Read(File::StreamRecord* file, void* data, int64_t offset, size_t* bytes) {
    // File descriptor must be initialized!
    BLIZZARD_ASSERT(file != nullptr && file->filehandle != INVALID_HANDLE_VALUE);

    if (bytes == nullptr || *bytes == 0) {
        return true;
    }

    if (offset > -1) {
        if (!File::SetPos(file, offset, BC_FILE_SEEK_START)) {
            return false;
        }
    }

    DWORD dwRead = 0;

    BOOL ok = ::ReadFile(file->filehandle, data, *bytes, &dwRead, nullptr);

    if (ok == 0) {
        // append any Win32 failure to the error log
        BC_FILE_SET_ERROR_MSG(BC_FILE_ERROR_BAD_FILE, "Win32 Read %p - %s", GetLastError(), file->path);
        return false;
    }

    *bytes = static_cast<size_t>(dwRead);

	return true;
}

bool Read(FileParms* parms) {
    return Read(parms->stream, parms->param, -1, &parms->size);
}

bool ReadP(FileParms* parms) {
    return Read(parms->stream, parms->param, parms->position, &parms->size);
}

bool RemoveDirectory(FileParms* parms) {
    auto dirpath = parms->filename;
    if (dirpath == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    File::Path::QuickNative pathNative(dirpath);

    BOOL ok = ::RemoveDirectory(dirpath);

    return ok != 0;
}

bool SetCacheMode(FileParms* parms) {
    auto file = parms->stream;

    if (file == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    if (file->filehandle == INVALID_HANDLE_VALUE) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_HANDLE);
        return false;
    }

    File::Path::QuickNative pathNative(file->path);

    // Close handle and reopen
    CloseHandle(file->filehandle);

    bool nocache = (parms->mode & File::Mode::nocache) != 0;

    file->filehandle = winfileutil::Open(pathNative.Str(), file->flags, nocache);

    if (file->filehandle == INVALID_HANDLE_VALUE) {
        BC_FILE_SET_ERROR_MSG(4, "Win32 SetCacheMode - %s", file->path);
        return false;
    }

    if (nocache) {
        parms->mode &= ~(File::Mode::nocache);
    }

    return true;
}

bool SetEOF(FileParms* parms) {
    auto file = parms->stream;

    if (file == nullptr || file->filehandle == INVALID_HANDLE_VALUE) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    // Save current pos
    int64_t originalpos = 0;

    if (!File::GetPos(file, originalpos)) {
        return false;
    }

    int64_t newpos = parms->position;
    auto    whence = parms->whence;

    if (!File::SetPos(file, newpos, whence)) {
        return false;
    }

    BOOL b = ::SetEndOfFile(file->filehandle);

    if (!b) {
        BC_FILE_SET_ERROR_MSG(4, "Win32 SetEOF - %s", file->path);
        return false;
    }

    // Restore original pos
    return File::SetPos(file, originalpos, BC_FILE_SEEK_START);
}

bool SetAttributes(FileParms* parms) {
    auto info = parms->info;

    if (info == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    auto    mode       = parms->mode;
    auto    attributes = info->attributes;
    int32_t status     = 0;
    auto    file       = parms->stream;
    auto    path       = parms->filename;

    if (mode & File::Mode::settimes) {
        auto modTime = info->modificationTime;

        FILETIME ft = winfileutil::PackFiletime(Time::ToWinFiletime(modTime));

        if (::SetFileTime(file->filehandle, nullptr, nullptr, &ft)) {
            file->info.modificationTime = modTime;

            parms->mode &= ~(File::Mode::settimes);
        }
    }

    if (mode & File::Mode::setperms) {
        File::Path::QuickNative pathNative(parms->filename);

        DWORD dwAttributes = winfileutil::AttributesToWin(info->attributes);

        BOOL ok = ::SetFileAttributes(pathNative.Str(), dwAttributes);

        if (!ok) {
            BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
            return false;
        }

        parms->mode &= ~(File::Mode::setperms);
    }

    return true;
}

bool SetPos(FileParms* parms) {
    auto file = parms->stream;

    if (file == nullptr || file->filehandle == INVALID_HANDLE_VALUE) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    auto offset = parms->position;
    auto whence = parms->whence;

    auto offsetLow  = static_cast<LONG>(offset);
    auto offsetHigh = static_cast<LONG>(offset >> 32);

    DWORD res = ::SetFilePointer(file->filehandle, offsetLow, &offsetHigh, static_cast<DWORD>(whence));
    if (res == INVALID_SET_FILE_POINTER) {
        res = GetLastError();
        if (res != 0) {
            return false;
        }
    }

    return true;
}

bool Delete(FileParms* parms) {
    auto deletePath = parms->filename;

    if (deletePath == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    File::Path::QuickNative deletePathNative(deletePath);

    BOOL ok = ::DeleteFile(deletePathNative.Str());

    return ok != 0;
}

bool Write(FileParms* parms) {
    auto file = parms->stream;
    auto data = parms->param;
    auto size = parms->size;

    if (file == nullptr || file->filehandle == INVALID_HANDLE_VALUE) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    DWORD numberOfBytesWritten = 0;

    BOOL ok = WriteFile(file->filehandle, data, size, &numberOfBytesWritten, nullptr);

    return ok != 0;
}

bool WriteP(FileParms* parms) {
    auto file     = parms->stream;
    auto data     = parms->param;
    auto position = parms->position;
    auto size     = parms->size;

    if (file == nullptr || file->filehandle == INVALID_HANDLE_VALUE) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    int64_t origpos = 0;

    if (!File::GetPos(file, origpos)) {
        return false;
    }

    if (!File::SetPos(file, position, BC_FILE_SEEK_START)) {
        return false;
    }

    DWORD numberOfBytesWritten = 0;
    BOOL ok = WriteFile(file->filehandle, data, size, &numberOfBytesWritten, nullptr);
    if (ok == 0) {
        BC_FILE_SET_ERROR_MSG(BC_FILE_ERROR_BAD_FILE, "Win32 Write - %s", file->path);

        return false;
    }

    return File::SetPos(file, origpos, BC_FILE_SEEK_START);
}

} // namespace Stacked
} // namespace System_File
} // namespace Blizzard

/********************************
* End of Win32 Stacked functions *
*********************************/
