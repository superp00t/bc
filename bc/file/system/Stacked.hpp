#ifndef BC_FILE_SYSTEM_STACKED_HPP
#define BC_FILE_SYSTEM_STACKED_HPP

#include "bc/file/Types.hpp"
#include "bc/file/Filesystem.hpp"
#include "bc/file/File.hpp"
#include "bc/file/system/System_File.hpp"

#include <cstdint>

namespace Blizzard {
namespace System_File {
namespace Stacked {

// Functions
bool Open(FileParms* parms);

bool file_init(File::Filesystem* fs, FileParms* parms);

File::Filesystem* Manager();

void HoistAll();

void Push(File::Filesystem* fs);

// Stacked file functions
bool SetWorkingDirectory(FileParms* parms);

bool Close(FileParms* parms);

bool GetWorkingDirectory(FileParms* parms);

bool ProcessDirFast(FileParms* parms);

bool Exists(FileParms* parms);

bool Flush(FileParms* parms);

bool GetFileInfo(FileParms* parms);

bool GetFreeSpace(FileParms* parms);

bool GetPos(FileParms* parms);

bool GetRootChars(FileParms* parms);

bool IsAbsolutePath(FileParms* parms);

bool IsReadOnly(FileParms* parms);

bool MakeAbsolutePath(FileParms* parms);

bool CreateDirectory(FileParms* parms);

bool Move(FileParms* parms);

bool Copy(FileParms* parms);

bool Open(FileParms* parms);

bool Read(FileParms* parms);

bool ReadP(FileParms* parms);

bool RemoveDirectory(FileParms* parms);

bool SetCacheMode(FileParms* parms);

bool SetEOF(FileParms* parms);

bool SetAttributes(FileParms* parms);

bool SetPos(FileParms* parms);

bool Delete(FileParms* parms);

bool Write(FileParms* parms);

bool WriteP(FileParms* parms);

} // namespace Stacked
} // namespace System_File
} // namespace Blizzard

#endif
