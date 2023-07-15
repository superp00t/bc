#ifndef BC_FILE_SYSTEM_WIN_UTILS_HPP
#define BC_FILE_SYSTEM_WIN_UTILS_HPP

#include "bc/file/Types.hpp"

#include <windows.h>
#include <cstdint>

namespace winfileutil {

// Translate Win32 file bits into Blizzard file bits.
uint32_t AttributesToBC(DWORD dwFileAttributes);

DWORD    AttributesToWin(uint32_t attributes);

// Returns true if BY_HANDLE_FILE_INFORMATION is successfully converted to File::FileInfo.
bool HandleFileInfoToBC(
    LPBY_HANDLE_FILE_INFORMATION  winInfo,
    Blizzard::File::FileInfo*     info);

// Returns true if WIN32_FILE_ATTRIBUTE_DATA is successfully converted to File::FileInfo.
bool AttributeFileInfoToBC(
    LPWIN32_FILE_ATTRIBUTE_DATA winInfo,
    Blizzard::File::FileInfo*     info);

HANDLE Open(const char* systemPath, uint32_t flags, bool nocache);

};

#endif
