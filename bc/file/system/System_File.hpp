#ifndef BC_FILE_SYSTEM_SYSTEM_FILE_HPP
#define BC_FILE_SYSTEM_SYSTEM_FILE_HPP

#include "bc/Debug.hpp"
#include "bc/file/system/Types.hpp"
#include "bc/file/Filesystem.hpp"

#if defined(WHOA_SYSTEM_WIN)
#include "bc/file/system/win/Support.hpp"
#endif

#include <cstdint>

namespace Blizzard {
namespace System_File {

// Fs calls
bool SetWorkingDirectory(File::Filesystem* fs, Stacked::FileParms* parms);
bool Close(File::Filesystem* fs, Stacked::FileParms* parms);
bool Create(File::Filesystem* fs, Stacked::FileParms* parms);
bool GetWorkingDirectory(File::Filesystem* fs, Stacked::FileParms* parms);
bool ProcessDirFast(File::Filesystem* fs, Stacked::FileParms* parms);
bool Exists(File::Filesystem* fs, Stacked::FileParms* parms);
bool Flush(File::Filesystem* fs, Stacked::FileParms* parms);
bool GetFileInfo(File::Filesystem* fs, Stacked::FileParms* parms);
bool GetFreeSpace(File::Filesystem* fs, Stacked::FileParms* parms);
bool GetPos(File::Filesystem* fs, Stacked::FileParms* parms);
bool GetRootChars(File::Filesystem* fs, Stacked::FileParms* parms);
bool IsAbsolutePath(File::Filesystem* fs, Stacked::FileParms* parms);
bool IsReadOnly(File::Filesystem* fs, Stacked::FileParms* parms);
bool MakeAbsolutePath(File::Filesystem* fs, Stacked::FileParms* parms);
bool CreateDirectory(File::Filesystem* fs, Stacked::FileParms* parms);
bool Move(File::Filesystem* fs, Stacked::FileParms* parms);
bool Copy(File::Filesystem* fs, Stacked::FileParms* parms);
bool Open(File::Filesystem* fs, Stacked::FileParms* parms);
bool Read(File::Filesystem* fs, Stacked::FileParms* parms);
bool ReadP(File::Filesystem* fs, Stacked::FileParms* parms);
bool RemoveDirectory(File::Filesystem* fs, Stacked::FileParms* parms);
bool SetCacheMode(File::Filesystem* fs, Stacked::FileParms* parms);
bool SetEOF(File::Filesystem* fs, Stacked::FileParms* parms);
bool SetAttributes(File::Filesystem* fs, Stacked::FileParms* parms);
bool SetPos(File::Filesystem* fs, Stacked::FileParms* parms);
bool Delete(File::Filesystem* fs, Stacked::FileParms* parms);
bool Write(File::Filesystem* fs, Stacked::FileParms* parms);
bool WriteP(File::Filesystem* fs, Stacked::FileParms* parms);
bool Shutdown(File::Filesystem* fs, Stacked::FileParms* parms);

} // namespace System_File
} // namespace Blizzard

#endif
