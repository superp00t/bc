#ifndef BC_FILE_DEFINES_HPP
#define BC_FILE_DEFINES_HPP

// How many bytes to when translating stacked filesystem names to large, native file paths
#define BC_FILE_MAX_PATH 1024

// File open/creation flags.
// See Blizzard::File::Open() for more info
#define BC_FILE_OPEN_LOCK           0x0001
#define BC_FILE_OPEN_OVERLAPPED     0x0002
#define BC_FILE_OPEN_READ           0x0004
#define BC_FILE_OPEN_WRITE          0x0008
#define BC_FILE_OPEN_TRUNCATE       0x0100
#define BC_FILE_OPEN_ALWAYS         0x0200
#define BC_FILE_OPEN_CREATE         0x0400
#define BC_FILE_OPEN_MUST_NOT_EXIST 0x0800
#define BC_FILE_OPEN_MUST_EXIST     0x1000

// File attribute flags
#define BC_FILE_ATTRIBUTE_READONLY  0x01
#define BC_FILE_ATTRIBUTE_HIDDEN    0x02
#define BC_FILE_ATTRIBUTE_SYSTEM    0x04
#define BC_FILE_ATTRIBUTE_ARCHIVE   0x08
#define BC_FILE_ATTRIBUTE_TEMPORARY 0x10
#define BC_FILE_ATTRIBUTE_NORMAL    0x20
#define BC_FILE_ATTRIBUTE_DIRECTORY 0x40

// File error codes
#define BC_FILE_ERROR_GENERIC_FAILURE       0
#define BC_FILE_ERROR_ACCESS_DENIED         1
#define BC_FILE_ERROR_FILE_NOT_FOUND        2
#define BC_FILE_ERROR_BAD_FILE              3
#define BC_FILE_ERROR_BUSY                  4
#define BC_FILE_ERROR_OOM                   5
#define BC_FILE_ERROR_INVALID_HANDLE        6
#define BC_FILE_ERROR_END_OF_FILE           7
#define BC_FILE_ERROR_INVALID_ARGUMENT      8
#define BC_FILE_ERROR_UNIMPLEMENTED         9
#define BC_FILE_ERROR_NO_SPACE_ON_DEVICE   11

// File SetPos whence
#define BC_FILE_SEEK_START   0
#define BC_FILE_SEEK_CURRENT 1
#define BC_FILE_SEEK_END     2

// Error handling utilities
#define BC_FILE_SET_ERROR(errorcode) ::Blizzard::File::SetLastError(static_cast<int32_t>(errorcode))
#define BC_FILE_SET_ERROR_MSG(errorcode, pfmt, ...) \
    ::Blizzard::File::SetLastError(errorcode); \
    ::Blizzard::File::AddToLastErrorStack(errorcode, ::Blizzard::String::QuickFormat<1024>(pfmt, __VA_ARGS__).Str(), 1)

#endif
