#include "bc/file/system/System_File.hpp"
#include "bc/file/system/Stacked.hpp"
#include "bc/file/File.hpp"
#include "bc/file/Defines.hpp"
#include "bc/Debug.hpp"
#include "bc/String.hpp"

#include <storm/String.hpp>

namespace Blizzard {
namespace System_File {

/******************************************
* Begin Win32 System_File stack functions *
*******************************************/

// Change current process's working directory to parms->filename.
bool SetWorkingDirectory(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::SetWorkingDirectory(parms);
}

// Close parms->stream file handle.
bool Close(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Close(parms);
}

bool Create(File::Filesystem* fs, Stacked::FileParms* parms) {
    // System_File::FileError(9)
    BC_FILE_SET_ERROR(BC_FILE_ERROR_UNIMPLEMENTED);
    return false;
}

bool GetWorkingDirectory(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::GetWorkingDirectory(parms);
}

bool ProcessDirFast(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::ProcessDirFast(parms);
}

bool Exists(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Exists(parms);
}

bool Flush(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Flush(parms);
}

bool GetFileInfo(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::GetFileInfo(parms);
}

bool GetFreeSpace(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::GetFreeSpace(parms);
}

bool GetPos(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::GetPos(parms);
}

bool GetRootChars(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::GetRootChars(parms);
}

bool IsAbsolutePath(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::IsAbsolutePath(parms);
}

bool IsReadOnly(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::IsReadOnly(parms);
}

bool MakeAbsolutePath(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::MakeAbsolutePath(parms);
}

bool CreateDirectory(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::CreateDirectory(parms);
}

bool Move(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Move(parms);
}

bool Copy(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Copy(parms);
}

bool Open(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Open(parms);
}

bool Read(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Read(parms);
}

bool ReadP(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::ReadP(parms);
}

bool RemoveDirectory(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::RemoveDirectory(parms);
}

bool SetCacheMode(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::SetCacheMode(parms);
}

bool SetEOF(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::SetEOF(parms);
}

bool SetAttributes(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::SetAttributes(parms);
}

bool SetPos(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::SetPos(parms);
}

bool Delete(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Delete(parms);
}

bool Write(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Write(parms);
}

bool WriteP(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::WriteP(parms);
}

bool Shutdown(File::Filesystem* fs, Stacked::FileParms* parms) {
    return false;
}

/****************************************
* End Win32 System_File stack functions *
*****************************************/

} // namespace System_File
} // namespace Blizzard
