#include "bc/file/system/System_File.hpp"
#include "bc/file/system/Stacked.hpp"
#include "bc/file/Path.hpp"
#include "bc/String.hpp"
#include "bc/Debug.hpp"
#include "bc/Memory.hpp"

#include <algorithm>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>

namespace Blizzard {
namespace System_File {

/*****************************************************
* Begin POSIX-compatible System_File stack functions *
******************************************************/

// Change current process's working directory to parms->filename.
bool SetWorkingDirectory(File::Filesystem* fs, Stacked::FileParms* parms) {
    BLIZZARD_ASSERT(parms->filename);

    auto len = Blizzard::String::Length(parms->filename) + 1;
    char wd[BC_FILE_MAX_PATH] = {0};
    File::Path::MakeNativePath(parms->filename, wd, len);

    return chdir(wd) == 0;
}

// Close parms->stream file descriptor.
bool Close(File::Filesystem* fs, Stacked::FileParms* parms) {
    auto file = parms->stream;
    BLIZZARD_ASSERT(file != nullptr);

    close(file->filefd);
    Memory::Free(file);

    return true;
}

// Maybe unimplemented because Open already implements a creation flag
// so Create is unecessary?
bool Create(File::Filesystem* fs, Stacked::FileParms* parms) {
    // System_File::FileError(9)
    BC_FILE_SET_ERROR(BC_FILE_ERROR_UNIMPLEMENTED);

    return false;
}

// Get the process's working directory to parms->directory.
// parms->directory comes with a cap specified by parms->directorySize.
bool GetWorkingDirectory(File::Filesystem* fs, Stacked::FileParms* parms) {
    if (parms->directory && parms->directorySize > 0) {
        *parms->directory = '\0';

        return getcwd(parms->directory, parms->directorySize) != 0;
    }

    // System_File::FileError(8)
    BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
    return false;
}

// iterate through directory calling back to parms->callback
bool ProcessDirFast(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::ProcessDirFast(parms);
}

// Boolean return here is simply whether a file (not a folder) Exists
bool Exists(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Exists(parms);
}

// Causes all modified data and attributes of file descriptor to be moved to
// a permanent storage device. This normally results in all in-core modified
// copies of buffers for the associated file to be written to a disk.
bool Flush(File::Filesystem* fs, Stacked::FileParms* parms) {
    auto file = parms->stream;
    BLIZZARD_ASSERT(file != nullptr);

#if defined(WHOA_SYSTEM_MAC)
    // from BSD syscall manual:
    // "Note that while fsync() will flush all data from the host to the drive
    //  (i.e. the "permanent storage device"), the drive itself may not physi-
    //  cally write the data to the platters for quite some time and it may be
    //  written in an out-of-order sequence."

    // "The F_FULLFSYNC fcntl asks the drive to flush all buffered data to permanent storage."
    auto status = fcntl(file->filefd, F_FULLFSYNC, 0);

    if (status == 0) {
        return true;
    }
#endif
    // Attempts to transfer all writes in the cache buffer to the underlying storage device.
    // fsync() does not return until the transfer is complete, or until an error is detected.
    return fsync(file->filefd) == 0;
}

// Attempt to request information about a filepath or an opened StreamRecord
bool GetFileInfo(File::Filesystem* fs, Stacked::FileParms* parms) {
    // Set up arguments
    auto            filename = parms->filename;
    struct stat     info;
    int32_t         status = -1;
    File::FileInfo* infoptr = parms->info;

    // Instead of a filename, a file descriptor can be supplied.
    if (filename == nullptr && parms->stream != nullptr) {
        // Info may be available from the file descriptor
        auto file = parms->stream;

        // Read stat from from stream fd if it exists
        if (file) {
            status = fstat(file->filefd, &info);
        }

        // If an info struct was not supplied,
        // Allow user to request file info pointer stored in StreamRecord
        if (infoptr == nullptr) {
            infoptr = &file->info;
            parms->info = infoptr;
        }
    } else {
        // Convert filename to native style.
        File::Path::QuickNative path(filename);
        // read stat from converted path
        status = stat(path.Str(), &info);
    }

    // Set existence bool
    auto exists = status != -1;

    // Collect POSIX filesystem info into File::FileInfo structure
    if (exists) {
        // Collect attributes.
        if (S_ISDIR(info.st_mode)) {
            infoptr->attributes |= BC_FILE_ATTRIBUTE_DIRECTORY;
        }

        if (S_ISREG(info.st_mode)) {
            infoptr->attributes |= BC_FILE_ATTRIBUTE_NORMAL;
        }

        mode_t readonly = 0444;

        if ((info.st_mode & readonly) == readonly) {
            infoptr->attributes |= BC_FILE_ATTRIBUTE_READONLY;
        }

        infoptr->device = static_cast<uint32_t>(info.st_dev);
        infoptr->size   = static_cast<uint64_t>(info.st_size);

        infoptr->accessTime                = Time::FromUnixTime(info.st_atime);
        infoptr->modificationTime          = Time::FromUnixTime(info.st_mtime);
        infoptr->attributeModificationTime = Time::FromUnixTime(info.st_ctime);
    }

    return true;
}

bool GetFreeSpace(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::GetFreeSpace(parms);
}

// Get current read/write position @parms->position
bool GetPos(File::Filesystem* fs, Stacked::FileParms* parms) {
    auto cur = lseek(parms->stream->filefd, 0, SEEK_CUR);
    if (cur == -1) {
        // FileError(8)
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    parms->position = static_cast<uint64_t>(cur);

    return true;
}

// Return an index @parms->position, pointing to the end of the "root chars" portion of a string.
// On MacOS, this would be under /Volumes/
bool GetRootChars(File::Filesystem* fs, Stacked::FileParms* parms) {
    auto name = parms->filename;

    if (*name == '/' || *name == '\\') {
        parms->position = 1;

#if defined(WHOA_SYSTEM_MAC)
        if (name != -1) {
            auto cmp = strncasecmp(name + 1, "Volumes", 7);
            auto volume = name + 9;

            auto ch = *volume;

            while(ch != '\0' && ch != '/' && ch != '\\') {
                ch = *volume++;
            }

            parms->position = reinterpret_cast<uintptr_t>(volume) - reinterpret_cast<uintptr_t>(name);

        }
#endif
    }

    return true;
}

// Returns true if parms->filename is not a relative path.
bool IsAbsolutePath(File::Filesystem* fs, Stacked::FileParms* parms) {
    return *parms->filename == '/' || *parms->filename == '\\';
}

// Return true if @parms->filename is readonly.
bool IsReadOnly(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::IsReadOnly(parms);
}

// Take source parms->filename path and prefix it with containing path.
bool MakeAbsolutePath(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::MakeAbsolutePath(parms);
}

// Create directory named parms->filename
bool CreateDirectory(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::CreateDirectory(parms);
}

// Move a file from parms->filename to parms->destination
bool Move(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Move(parms);
}

// Copy file data from parms->filename to parms->destination
// Must only return true if file was copied entirely (Move depends on this being accurate)
bool Copy(File::Filesystem* fs, Stacked::FileParms* parms) {
    auto overwrite = parms->flag;

    // Set up virtual paths
    auto source      = parms->filename;
    auto destination = parms->destination;

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
    size_t sz_copybuffer = std::min(sz_source, BC_FILE_SYSTEM_COPYBUFFER_SIZE);
    auto   u8_copybuffer = reinterpret_cast<uint8_t*>(Memory::Allocate(sz_copybuffer));

    // Loop through the source file, reading segments into copybuffer
    for (size_t index = 0; index < sz_source; index += sz_copybuffer) {
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

bool Open(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Open(parms);
}

bool Read(File::StreamRecord* file, void* data, int64_t offset, size_t* bytes) {
    // File descriptor must be initialized!
    BLIZZARD_ASSERT(file != nullptr && file->filefd != -1);

    if (bytes == nullptr || *bytes == 0) {
        return true;
    }

    // in bytes, the length of the user's read operation.
    auto    size   = *bytes;
    // byte index.
    int32_t index  = 0;
    // read status (or count of bytes read)
    int32_t result = 0;

    // Fully read the input file according to the count requested.
    // Partial reads will be merged together in the end.
    while (index < size) {
        // Slice length
        auto slice = size - index;

        if (offset < 0) {
            // Read relative to file pointer
            result = read(file->filefd, data, slice);
        } else {
            // Read absolute
            result = pread(file->filefd, data, slice, offset);
        }

        auto increment = result;
        if (increment < 1) {
            increment = 0;
        }

        if (offset >= 0) {
            offset += increment;
        }

        index += increment;

        if (result < 0) {
            // append any POSIX failure to the error log
            BC_FILE_SET_ERROR_MSG(100 + errno, "Posix Read - %s", file->path);
            return false;
        }

        if (result == 0) {
            // append unexpected EOF to the error log
            BC_FILE_SET_ERROR_MSG(BC_FILE_ERROR_END_OF_FILE, "Posix Read - End of File - %s", file->path);
            return false;
        }

        data += increment;
    }

    // How many bytes did we read
    *bytes = size - index;

	return true;
}

// Read from parms->stream to void* buffer(parms->param)
// Reading up to #parms->size bytes
// parms->size also stores the result of how many bytes were actually read
bool Read(File::Filesystem* fs, Stacked::FileParms* parms) {
    // Position is negative, so actual write position is the current seek offset of the fd.
    return Read(parms->stream, parms->param, -1, &parms->size);
}

// Specify a file position when reading from a file
bool ReadP(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Read(parms->stream, parms->param, parms->position, &parms->size);
}

// Remove an directory
bool RemoveDirectory(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::RemoveDirectory(parms);
}

// Specify disk caching preferences for accessing stream @parms->stream
bool SetCacheMode(File::Filesystem* fs, Stacked::FileParms* parms) {
    auto mode = parms->mode;
    auto file = parms->stream;

#if defined(WHOA_SYSTEM_MAC)
    // Require user to have clean flags (weird)
    BLIZZARD_ASSERT(0 == (mode & ~File::Mode::nocache));

    File::Flush(file);

    return 0 == fcntl(file->filefd, F_NOCACHE, mode & File::Mode::nocache);
#endif

#if defined(WHOA_SYSTEM_LINUX)
    // TODO: implement equivalent fcntl
    return false;
#endif
}

// Set end of file to parms->position.
bool SetEOF(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::SetEOF(parms);
}

// Set file permissions and read/write modes.
bool SetAttributes(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::SetAttributes(parms);
}

// Set the file pointer
bool SetPos(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::SetPos(parms);
}

// Delete a file
bool Delete(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Stacked::Delete(parms);
}

// Write system file specifying options.
bool Write(File::StreamRecord* file, void* data, int64_t offset, int32_t* bytes) {
    // File descriptor must be initialized!
    BLIZZARD_ASSERT(file != nullptr && file->filefd != -1);

    if (bytes == nullptr || *bytes == 0) {
        return true;
    }

    // in bytes, the length of the user's write operation.
    auto    size   = *bytes;
    // byte index.
    int32_t index  = 0;
    // write status (or count of bytes written)
    int32_t result = 0;

    // Fully write data buffer to file.
    while (index < size) {
        // Slice length
        auto slice = size - index;

        if (offset < 0) {
            // Write relative to current file pointer
            result = write(file->filefd, data, slice);
        } else {
            // Write at an absolute file position
            result = pwrite(file->filefd, data, slice, offset);
        }

        auto increment = result;
        if (increment < 1) {
            increment = 0;
        }

        if (offset >= 0) {
            offset += increment;
        }

        index += increment;

        if (result == -1) {
            // append any POSIX failure to the error log
            BC_FILE_SET_ERROR_MSG(100 + errno, "Posix Write - %s", file->path);
            return false;
        }

        data += increment;
    }

    // How many bytes did we read
    *bytes = size - index;

	return true;
}

// Write parms->param data
bool Write(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Write(parms->stream, parms->param, -1, &parms->size);
}

bool WriteP(File::Filesystem* fs, Stacked::FileParms* parms) {
    return Write(parms->stream, parms->param, parms->position, &parms->size);
}

bool Shutdown(File::Filesystem* fs, Stacked::FileParms* parms) {
    // ?
    return true;
}

/***************************************************
* End POSIX-compatible System_File stack functions *
****************************************************/

} // namespace System_File
} // namespace Blizzard
