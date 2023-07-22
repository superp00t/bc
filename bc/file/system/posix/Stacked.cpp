#include "bc/file/system/Stacked.hpp"
#include "bc/file/Path.hpp"
#include "bc/Debug.hpp"
#include "bc/Memory.hpp"
#include "bc/String.hpp"

#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

namespace Blizzard {
namespace System_File {
namespace Stacked {

bool Open(FileParms* parms) {
    File::Path::QuickNative pathNative(parms->filename);

    // Build POSIX flags based on flags from FileParms
    bool read         = parms->flag & BC_FILE_OPEN_READ;
    bool write        = parms->flag & BC_FILE_OPEN_WRITE;
    bool mustNotExist = parms->flag & BC_FILE_OPEN_MUST_NOT_EXIST;
    bool mustExist    = parms->flag & BC_FILE_OPEN_MUST_EXIST;
    bool create       = parms->flag & BC_FILE_OPEN_CREATE;
    bool truncate     = parms->flag & BC_FILE_OPEN_TRUNCATE;

    BLIZZARD_ASSERT(read || write);

    int32_t flags = 0;

    if (read && !write) {
        flags = O_RDONLY;
    } else if (!read && write) {
        flags = O_WRONLY;
    } else if (read && write) {
        flags = O_RDWR;
    }

    int32_t fd = -1;

    if (create) {
        flags |= O_CREAT;

        if (mustNotExist) {
            flags |= O_EXCL;
        }

        fd = ::open(pathNative.Str(), flags, 511);
    } else {
        fd = ::open(pathNative.Str(), flags);
    }

    if (fd == -1) {
        BC_FILE_SET_ERROR_MSG(100 + errno, "Posix Open - %s", parms->filename);
        return false;
    }

    // Successfully opened file handle. Allocate StreamRecord + path str at the end.
    auto recordSize = (sizeof(File::StreamRecord) - File::StreamRecord::s_padPath) + (1 + pathNative.Size());
    auto fileData = Memory::Allocate(recordSize);
    if (fileData == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_OOM);
        return false;
    }

    String::MemFill(fileData, recordSize, 0);

    auto file = reinterpret_cast<File::StreamRecord*>(fileData);

    file->flags  = flags;
    file->filefd = fd;

    String::Copy(file->path, parms->filename, pathNative.Size());

    File::GetFileInfo(file);

    parms->stream = file;

    return true;
}

bool Exists(FileParms* parms) {
    BC_FILE_PATH(buffer);

    auto        filepath = parms->filename;
    auto        empty    = "";
    size_t      len      = 0;
    struct stat info     = {};
    bool        exists   = false;

    File::Path::QuickNative filepathNative(filepath);

    auto status = ::stat(filepathNative.Str(), &info);

    if (status != -1) {
        parms->info->attributes = 0;
        // Collect attributes.
        if (S_ISDIR(info.st_mode)) {
            parms->info->attributes |= BC_FILE_ATTRIBUTE_DIRECTORY;
        }

        if (S_ISREG(info.st_mode)) {
            exists = true;
            parms->info->attributes |= BC_FILE_ATTRIBUTE_NORMAL;
        }
    }

    return exists;
}

bool GetFreeSpace(FileParms* parms) {
    auto dirpath = parms->filename;

    if (dirpath == nullptr || *dirpath == '\0') {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    File::Path::QuickNative dirpathNative(dirpath);

    struct statvfs sv;
    if (::statvfs(dirpathNative.Str(), &sv) != 0) {
        BC_FILE_SET_ERROR_MSG(BC_FILE_ERROR_INVALID_ARGUMENT, "Posix GetFreeSpace - %s", dirpath);
        return false;
    }

    parms->size64 = sv.f_bavail * sv.f_frsize;
    return true;
}

bool ProcessDirFast(FileParms* parms) {
    auto dirpath = parms->filename;
    File::Path::QuickNative dirpathNative(dirpath);

    // Call back to this function when processing
    File::ProcessDirCallback callback = parms->callback;

    // Open directory
    auto directory = ::opendir(dirpathNative.Str());
    if (!directory) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    // Stores names temporarily
    char name[256] = {0};

    bool status = false;

    File::ProcessDirParms walkparms = {};
    walkparms.root  = parms->filename;
    walkparms.param = parms->param;
    walkparms.item  = name;

    struct dirent* ent = nullptr;

    while ((ent = ::readdir(directory)) != nullptr) {
        String::Copy(name, ent->d_name, 256);

        auto isDotCurrent = (name[0] == '.' && name[1] == '\0');
        auto isDotParent  = (name[0] == '.' && name[1] == '.' && name[2] == '\0');

        if (!(isDotCurrent || isDotParent)) {
            walkparms.itemIsDirectory = ent->d_type == DT_DIR;
            status = callback(walkparms);
            if (status) {
                break;
            }
        }
    }

    ::closedir(directory);
    return status;
}

bool IsReadOnly(FileParms* parms) {
    auto manager  = Manager();
    auto filename = parms->filename;

    if (!filename || !manager || !File::Exists(filename)) {
        // FileError(8)
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    File::StreamRecord* stream = nullptr;

    auto flags = BC_FILE_OPEN_LOCK | BC_FILE_OPEN_WRITE | BC_FILE_OPEN_MUST_EXIST;

    bool opened = File::Open(filename, flags, stream);
    if (!opened) {
        return true;
    }

    File::Close(stream);
    return false;
}

// realpath() based
bool MakeAbsolutePath(FileParms* parms) {
    auto unkflag = parms->flag;

    BC_FILE_PATH(basepathfast);
    BC_FILE_PATH(univpathfast);
    char* basepath = nullptr;
    char* univpath = nullptr;

    if (parms->directorySize < BC_FILE_MAX_PATH) {
        basepath = basepathfast;
    } else {
        basepath = reinterpret_cast<char*>(Memory::Allocate(parms->directorySize));
    }

    if (!File::IsAbsolutePath(parms->filename)) {
        // If the path is not absolute already, pack current working dir to the base path.
        File::GetWorkingDirectory(basepath, parms->directorySize);

        // Force a slash to the end of the base path, so that we can append univpath
        File::Path::ForceTrailingSeparator(basepath, parms->directorySize, BC_FILE_SYSTEM_PATH_SEPARATOR);
    }

    String::Append(basepath, parms->filename, parms->directorySize);

    if (parms->directorySize <= BC_FILE_MAX_PATH) {
        univpath = univpathfast;
    } else {
        univpath = reinterpret_cast<char*>(Memory::Allocate(parms->directorySize));
    }

    File::Path::MakeNativePath(basepath, univpath, parms->directorySize);

    const char*  local_1060 = nullptr;
    const char*  local_1054 = nullptr;
    char*        temp_directory = nullptr;
    size_t       temp_size = 0;
    char*        copied_univ_path = nullptr;
    char*        previous_character = nullptr;
    char*        next_slash         = nullptr;
    BC_FILE_PATH(next_slash_fast);
    BC_FILE_PATH(temp_directory_fast);
    char         current_byte = 0;
    size_t       len = 0;
    size_t       copied_len = 0;

    auto after_first_slash = univpath + 1;
    loop_start:
    do {
        next_slash = String::Find(after_first_slash, '/', parms->directorySize);
        if (next_slash == nullptr) {
            len = String::Length(univpath);
            if (((len > 2) && (univpath[len - 1] == '.')) && (univpath[len - 2] == '/')) {
                univpath[len - 1] = '\0';
            }

            if (unkflag) {
                temp_size = parms->directorySize;

                if (temp_size <= BC_FILE_MAX_PATH) {
                    temp_directory = temp_directory_fast;
                } else {
                    temp_directory = reinterpret_cast<char*>(Memory::Allocate(temp_size));
                }

                local_1060 = temp_directory;
                local_1054 = univpath;

                after_first_slash = univpath;
                copied_univ_path = temp_directory;
                eat_byte:
                current_byte = *after_first_slash;
                if (current_byte != '\0') {
                    current_byte_must_be_forward_slash:
                    if (current_byte != '/') {
                        break;
                    }
                    goto do_workingdir_buffer_realpath;
                }

                copy_universalpath:
                String::Copy(univpath, copied_univ_path, parms->directorySize);
                if ((copied_univ_path != local_1060) && (copied_univ_path != nullptr)) {
                    Memory::Free(copied_univ_path);
                }
            }

            String::Copy(parms->directory, univpath, parms->directorySize);
            current_byte = *parms->directory;

            if (basepath != basepathfast && basepath != nullptr) {
                // 0x1a211d
                Memory::Free(basepath);
            }
            if (univpath != univpathfast && univpath != nullptr) {
                // 0x1a2137
                Memory::Free(univpath);
            }

            return current_byte != '\0';
        }

        if (next_slash[1] != '.') {
            copy_or_increment_resume_loop:
            if ((univpath < previous_character) && (after_first_slash == next_slash)) {
                String::Copy(next_slash,next_slash + 1,parms->directorySize);
            } else {
                after_first_slash = next_slash + 1;
                previous_character = next_slash;
            }
            goto loop_start;
        }

        if (next_slash[2] == '.') {
            if (next_slash[3] != '/') {
                goto copy_or_increment_resume_loop;
            }
            String::Copy(after_first_slash, next_slash + 4, parms->directorySize);
        } else {
            if (next_slash[2] != '/') {
                goto copy_or_increment_resume_loop;
            }
            String::Copy(next_slash + 1, next_slash + 3, parms->directorySize);
        }
    } while(true);

    after_first_slash = after_first_slash + 1;
    current_byte = *after_first_slash;

    if (current_byte == '\0') {
        do_workingdir_buffer_realpath:
        previous_character = after_first_slash + 1;
        String::Copy(temp_directory, local_1054, (static_cast<int32_t>(previous_character - local_1054)) + 1);
        if (parms->directorySize <= BC_FILE_MAX_PATH) {
            next_slash = next_slash_fast;
        } else {
            next_slash = reinterpret_cast<char*>(Memory::Allocate(parms->directorySize));
        }


        if (::realpath(copied_univ_path, next_slash) == nullptr) {
            temp_directory = temp_directory + (static_cast<int32_t>(previous_character - local_1054));
        } else {
            String::Copy(copied_univ_path,next_slash,parms->directorySize);
            if ((*after_first_slash == '/') || ((*after_first_slash == '\0' && (after_first_slash[-1] == '/')))) {
                File::Path::ForceTrailingSeparator(copied_univ_path, parms->directorySize, '/');
            }
            temp_directory = copied_univ_path;
            copied_len = String::Length(copied_univ_path);
            temp_directory = temp_directory + copied_len;
        }

        if (*after_first_slash != '\0') {
            after_first_slash = previous_character;
            local_1054 = previous_character;
        }

        if ((next_slash == next_slash_fast) || (next_slash == nullptr)) {
            goto eat_byte;
        }

        Memory::Free(next_slash);
        current_byte = *after_first_slash;
        if (current_byte == '\0') {
            goto copy_universalpath;
        }
    }
    goto current_byte_must_be_forward_slash;
}

// Create a full directory path
bool CreateDirectory(FileParms* parms) {
    if (parms->filename == nullptr) {
        // FileError(8)
        BC_FILE_SET_ERROR(8);
        return false;
    }

    char        tmp[BC_FILE_MAX_PATH] = {};
    char*       p = nullptr;
    struct stat sb;

    // Copy path
    File::Path::MakeNativePath(parms->filename, tmp, BC_FILE_MAX_PATH);

    auto len = String::Length(tmp);

    // Remove trailing slash
    if(tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    // check if path exists and is a directory
    if (::stat(tmp, &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) {
            return true;
        }
    }

    // Loop through path and call mkdir on path elements
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            // test path
            if (::stat(tmp, &sb) != 0) {
                // path does not exist, create directory
                if (::mkdir(tmp, 511) < 0) {
                    return false;
                }
            } else if (!S_ISDIR(sb.st_mode)) {
                // not a directory
                return -1;
            }
            *p = '/';
        }
    }

    // check remaining path existence
    if (::stat(tmp, &sb) != 0) {
        // path does not exist, create directory
        if (::mkdir(tmp, 511) < 0) {
            return false;
        }
    } else if (!S_ISDIR(sb.st_mode)) {
        // not a directory
        return false;
    }
    return true;
}

bool Move(FileParms* parms) {
    File::Path::QuickNative source(parms->filename);
    File::Path::QuickNative destination(parms->destination);

    struct stat st;

    // Fail if destination already exists.
    int32_t status = ::stat(destination.Str(), &st);
    if (status == 0) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    // See if we can just rename the file. Pretty fast if we can avoid copying.
    status = ::rename(source.Str(), destination.Str());
    if (status == 0) {
        return true;
    }

    // If rename failed due to cross-device linking (can't rename to a different device)
    // Copy the file from one device to another (slow)
    if (errno == EXDEV) {
        if (File::Copy(parms->filename, parms->destination, false)) {
            // Source is deleted once File::Copy is successful.
            File::Delete(parms->filename);
            return true;
        }
        return true;
    }

    return false;
}

// Attempts to remove an empty directory.
bool RemoveDirectory(FileParms* parms) {
    auto dir = parms->filename;

    // Must have directory path to remove directory.
    if (!dir) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    // Convert to native path.
    File::Path::QuickNative dirNative(dir);

    // Attempt rmdir
    return ::rmdir(dirNative.Str()) == 0;
}

// Change file EOF to a new offset @ parms->position and according to parms->whence.
// DESTROYING any existing data at offset >= parms->position
bool SetEOF(FileParms* parms) {
    auto file   = parms->stream;

    // Seek to truncation point based on position and whence
    // then, read computed truncation point into parms->position
    if (!File::SetPos(file, parms->position, parms->whence) ||
        !File::GetPos(file, parms->position)) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    // Perform truncation.
    auto status = ::ftruncate(file->filefd, static_cast<off_t>(parms->position));
    if (status != -1) {
        // Success!

        // presumably this is to invalidate stream's fileinfo (which is no longer recent)
        file->hasInfo = false;
        return true;
    }

    // Some POSIX error has occurred.

    // Can occur as user may have tried to to resize to a large parms->position
    if (errno == ENOSPC) {
        // Code(9)
        BC_FILE_SET_ERROR(BC_FILE_ERROR_NO_SPACE_ON_DEVICE);
        return false;
    }

    BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
    return false;
}

// Changes file attributes of object at parms->filename
// Sets filemode with chmod()
// Changes update times with futimes()
bool SetAttributes(FileParms* parms) {
    BC_FILE_PATH(path);

    auto info = parms->info;

    if (info == nullptr) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    uint32_t   mode       = parms->mode;
    auto       attributes = info->attributes;
    int32_t    status     = 0;
    auto       file       = parms->stream;

    if (mode & File::Mode::settimes) {
#if defined(WHOA_SYSTEM_MAC)
        // Use BSD function futimes
        struct timeval tvs[2];

        tvs[0].tv_sec  = Time::ToUnixTime(info->modificationTime);
        tvs[0].tv_usec = 0;

        tvs[1].tv_sec  = tvs[0].tv_sec;
        tvs[1].tv_usec = 0;

        // Attempt to apply times to file descriptor.
        status = ::futimes(file->filefd, tvs);
#else
        // use Linux equivalent futimens
        struct timespec tsp[2];
        tsp[0].tv_sec  = Time::ToUnixTime(info->modificationTime);
        tsp[0].tv_nsec = 0;

        tsp[1].tv_sec  = tsp[0].tv_sec;
        tsp[1].tv_nsec = 0;
        status = ::futimens(file->filefd, tsp);
#endif

        if (status != 0) {
            BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
            return false;
        }

        // If successful, also apply these changes to FileInfo inside StreamRecord.
        file->info.accessTime       = info->modificationTime;
        file->info.modificationTime = info->modificationTime;
        parms->mode &= ~(File::Mode::settimes);
    }

    if (mode & File::Mode::setperms) {
        File::Path::QuickNative path(parms->filename);

        // Get unix permissions
        struct stat info = {};
        auto status      = ::stat(path.Str(), &info);
        if (status == -1) {
            // Can't set attributes on a nonexistent file ૮ ・ﻌ・ა
            return false;
        }

        if (attributes & BC_FILE_ATTRIBUTE_READONLY) {
            status = ::chmod(path.Str(), 444);
        } else {
            status = ::chmod(path.Str(), 511);
        }

        if (status != 0) {
            BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
            return false;
        }

        parms->mode &= ~(File::Mode::setperms);
    }

    return true;
}

} // namespace Stacked
} // namespace System_File
} // namespace Blizzard
