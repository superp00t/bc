#ifndef BC_FILE_FILE_HPP
#define BC_FILE_FILE_HPP

#include "bc/Time.hpp"
#include "bc/file/Defines.hpp"
#include "bc/file/Types.hpp"
#include "bc/file/system/System_File.hpp"

namespace Blizzard {
namespace File {

// Functions
void SetLastError(int32_t errorcode);

void AddToLastErrorStack(int32_t errorcode, const char* msg, int32_t param_3);

bool Copy(const char* source, const char* destination, bool overwrite);

bool Delete(const char* path);

bool Exists(const char* path);

bool IsFile(const char* path);

bool IsDirectory(const char* path);

bool      GetFileInfo(const char* path, FileInfo* info);
bool      GetFileInfo(StreamRecord* stream, FileInfo* info);
FileInfo* GetFileInfo(StreamRecord* stream);

bool GetWorkingDirectory(char* path, size_t capacity);

bool MakeAbsolutePath(const char* rel, char* result, int32_t capacity, bool unkflag);

bool Open(const char* filename, uint32_t flags, StreamRecord*& stream);

bool Close(StreamRecord* stream);

bool SetAttributes(const char* filename, uint32_t attributes);

bool Read(StreamRecord* stream, void* buffer, size_t bytesToRead, size_t* bytesRead, int32_t flag);

bool Write(StreamRecord* stream, void* buffer, size_t bytesToWrite, size_t* bytesWritten, int32_t flag);

bool SetPos(StreamRecord* stream, int64_t pos, int32_t whence);

bool GetPos(StreamRecord* stream, int64_t& pos);

} // namespace File
} // namespace Blizzard

#endif
