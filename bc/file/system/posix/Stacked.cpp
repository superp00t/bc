#include "bc/file/system/Stacked.hpp"
#include "bc/file/Path.hpp"
#include "bc/Debug.hpp"
#include "bc/Memory.hpp"

#include <fcntl.h>
#include <sys/stat.h>

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

    int32_t flag = 0;

    if (read && !write) {
        flag = O_RDONLY;
    } else if (!read && write) {
        flag = O_WRONLY;
    } else if (read && write) {
        flag = O_RDWR;
    }

    int32_t fd = -1;

    if (create) {
        flag |= O_CREAT;

        if (mustNotExist) {
            flag |= O_EXCL;
        }

        fd = open(pathNative.Str(), flag, 511);
    } else {
        fd = open(pathNative.Str(), flag);
    }

    bool success = fd != -1;

    if (!success) {
        BC_FILE_SET_ERROR_MSG(100 + errno, "Posix Open - %s", systemPath);
        return false;
    }

    return true;
}

bool Exists(FileParms* parms) {
    BC_FILE_PATH(buffer);

    auto        filepath = parms->filename;
    char*       empty    = "";
    size_t      len      = 0;
    struct stat info     = {};
    bool        exists   = false;

    File::Path::QuickNative filepathNative(filepath);

    auto status = stat(filepathNative.Str(), &info);

    bool exists = false;
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
    if (statvfs(dirpathNative.Str(), &sv) != 0) {
        BC_FILE_SET_ERROR_MSG(BC_FILE_ERROR_INVALID_ARGUMENT, "Posix GetFreeSpace - %s", parms->path);
        return false;
    }

    parms->size64 = sv.f_bavail * sv.f_blocks;
    return true;
}

bool ProcessDirFast(FileParms* parms) {
    auto dirpath = parms->filename;
    File::Path::QuickNative dirpathNative(dirpath);

    // Call back to this function when processing
    File::ProcessDirCallback callback = parms->callback;

    // Open directory
    auto directory = opendir(dirpathNative.Str());
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

    while ((ent = readdir(directory)) != nullptr) {
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

    closedir(directory);
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

bool MakeAbsolutePath(FileParms* parms) {
    // char wd[BC_FILE_MAX_PATH]     = {0};
    // char buffer[BC_FILE_MAX_PATH] = {0};
    // auto name                                 = parms->filename;


    // if (!File::IsAbsolutePath(name)) {
    //     // If the path is not absolute already, pack current working dir to the base path.
    //     File::GetWorkingDirectory(basepath, parms->directorySize);

    //     // Force a slash to the end of the base path, so that we can append univpath
    //     String::ForceTrailingSeparator(basepath, parms->directorySize, BC_FILE_SYSTEM_PATH_SEPARATOR);
    }

    return true;
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
    size_t len;

    File::Path::MakeNativePath(parms->filename, tmp, BC_FILE_MAX_PATH);

    // Copy path
    len = String::Length(tmp);
    if (len == 0 || len == PATH_MAX_STRING_SIZE) {
        return false;
    }

    String::MemCopy(tmp, dir, len);
    tmp[len] = '\0';

    // Remove trailing slash
    if(tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    // check if path exists and is a directory
    if (stat(tmp, &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) {
            return true;
        }
    }

    // Loop through path and call mkdir on path elements
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            // test path
            if (stat(tmp, &sb) != 0) {
                // path does not exist, create directory
                if (mkdir(tmp, 511) < 0) {
                    return false;
                }
            } else if (!S_ISDIR(sb.st_mode)) {
                // not a directory
                return -1;
            }
            *p = '/';
        }
    }
    if (stat(tmp, &sb) != 0) {
        // path does not exist, create directory
        if (mkdir(tmp, mode) < 0) {
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

    // Fail if destination already exists.
    int32_t status = stat(destination.Str(), &destInfo);
    if (status == 0) {
        BC_FILE_SET_ERROR(BC_FILE_ERROR_INVALID_ARGUMENT);
        return false;
    }

    // See if we can just rename the file. Pretty fast if we can avoid copying.
    status = rename(source.Str(), destination.Str());
    if (status == 0) {
        return true;
    }

    // If rename failed due to cross-device linking (can't rename to a different device)
    // Copy the file from one device to another
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
    return rmdir(dirNative.Str()) == 0;
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
    auto status = ftruncate(file->filefd, static_cast<off_t>(parms->position));
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
        BC_FILE_SET_ERROR(BC_FILE_ERROR_NO_SPACE);
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

    File::Mode mode       = parms->mode;
    auto       attributes = info->attributes;
    int32_t    status     = 0;
    auto       file       = parms->stream;

    if (mode & File::Mode::settimes) {
        timeval tvs[2];

        tvs[0].tv_sec  = Time::ToUnixTime(info->modficationTime);
        tvs[0].tv_nsec = 0;

        tvs[1].tv_sec  = tvs[0].tv_sec
        tvs[1].tv_nsec = 0;

        // Attempt to apply times to file descriptor.
        status = futimes(file->filefd, tvs);
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
        auto status      = stat(path.Str(), &info);
        if (status == -1) {
            // Can't set attributes on a nonexistent file ૮ ・ﻌ・ა
            return false;
        }

        if (attributes & BC_FILE_ATTRIBUTE_READONLY) {
            status = chmod(path, 444);
        } else {
            status = chmod(path, 511);
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
