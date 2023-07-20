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

        fd = open(pathNative.Str(), flags, 511);
    } else {
        fd = open(pathNative.Str(), flags);
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

    if (parms->directorySize < BC_FILE_MAX_PATH+1) {
        univpath = univpathfast;
    } else {
        univpath = reinterpret_cast<char*>(Memory::Allocate(parms->directorySize));
    }

    File::Path::MakeNativePath(basepath, univpath, parms->directorySize);

    auto post_slash = univpath + 1;

    auto found = String::Find(post_slash, '/', parms->directorySize);

    auto v22 = found;
    char* v18 = basepath;
    char* v23;
    const char* v24 = post_slash;
    const char* v25 = univpath;
    const char* v26 = nullptr;
    const char* v27 = nullptr;
    const char* v28;
    const char* v29;
    const char* v30;
    size_t v31;
    char v32;

    if (found != nullptr) {
        while (true) {
            // 0x1a1e94
            v28 = univpath;
            v29 = post_slash;
            v27 = post_slash;
            v30 = v29;
            v26 = found;
            while (true) {
              lab_0x1a1e94:
                // 0x1a1e94
                v25 = v26;
                v24 = v25 + 1;
                v23 = (char *)v24;
                if (*v23 != 46) {
                    goto lab_0x1a1e60;
                } else {
                    v32 = *(v25 + 2); // 0x1a1e9d
                    if (v32 == 46) {
                        // 0x1a1ec7
                        if (*(v25 + 3) != 47) {
                            goto lab_0x1a1e60;
                        } else {
                            // 0x1a1ecd
                            String::Copy(v27, (v25 + 4), parms->directorySize);
                            goto lab_0x1a1e71;
                        }
                    } else {
                        if (v32 != 47) {
                            goto lab_0x1a1e60;
                        } else {
                            // 0x1a1ea9
                            String::Copy(v23, (char *)(v25 + 3), parms->directorySize);
                            goto lab_0x1a1e71;
                        }
                    }
                }
            }
          lab_0x1a1e71_2:
            // 0x1a1e71
            v22 = String::Find(v23, 47, parms->directorySize);
            if (v22 == 0) {
                // break -> 0x1a1f09
                break;
            }
        }
    }

lab_0x1a1f09:
    auto v33 = String::Length(univpath); // 0x1a1f12
    if (v33 >= 3) {
        char*  v34 = v33 + univpath;
        char * v35 = (char *)(v34 - 1); // 0x1a1f26
        if (*v35 == 46) {
            // 0x1a2216
            if (*(char *)(v34 - 2) == 47) {
                // 0x1a2221
                *v35 = 0;
            }
        }
    }

    // 0x1a1f2f
    char * v36;
    char*  v37;
    char * v38;
    char*  v39;
    char*  v40;
    char*  v41;
    char*  v42;
    char*  v43;
    char*  v44;
    char*  v45;
    char*  v46;
    char*  v47;
    char v48;
    char*  v49;
    char*  v50;
    char* v51;
    char*  v52;
    char v53[1024] = {0}; // bp-4152, 0x1a1d50
    char* v54;
    char* v55;
    char*  v56;
    char*  v57;
    char*  v58;
    char*  v59;
    char*  v60; // 0x1a1f9c
    char* v61;
    size_t v62;
    char*  v63;
    char v64[1024] = {0}; // bp-3120, 0x1a1d50
    char* v65;
    void* v67;
    char  v68;
    char*  v69;
    char*  v70;
    char*  v71;
    char v72; // 0x1a1f79
    char* v73;
    char * v74;
    char*  v75;
    char*  v76;
    char*  v77;
    char*  v78;
    char*  v79;
    char v80; // 0x1a1f95
    char*  v81;
    char*  v82; // 0x1a1f92
    char*  v83;
    char*  v84;
    size_t v85;
    size_t v86;
    char*  v87;
    char   v89;
    char v95;

    if (unkflag) {
        v62 = parms->directorySize; // 0x1a1f3f
        char* v63;
        char*  v65;
        if (v62 < 1025) {
            v38 = v64;
            v63 = v64;
            v65 = v64;
        } else {
            v67 = reinterpret_cast<char*>(Memory::Allocate((int64_t)v62)); // 0x1a21f8
            v38 = v67;
            v63 = v67;
            v65 = v64;
        }
        v68 = *v18; // 0x1a1f79
        if (v68 == 0) {
            // 0x1a1f67
            v37 = v38;
        } else {
            // 0x1a1f90
            v59 = v38;
            v69 = v63;
            v70 = v63;
            v71 = v63;
            v72 = v68; // 0x1a1f79
            v73 = v18;
            v74 = v18;
            v75 = v63;
            while (true) {
                // 0x1a1f90
                v39 = v69;
                v40 = v70;
                v41 = v71;
                v56 = v75;
                v55 = v74;
                v48 = v72;
                v49 = v73;
                while (true) {
                  lab_0x1a1f90:;
                    v76 = v56;
                    v77 = v41;
                    v78 = v40;
                    v79 = v39;
                    v80 = v48; // 0x1a1f95
                    v81 = v49;
                    v82 = v81; // 0x1a1f92
                    while (v80 != 47) {
                        v83 = v81 + 1; // 0x1a1f94
                        v80 = *(char *)v83;
                        v82 = v83;
                        if (v80 == 0) {
                            // break -> 0x1a1f9c
                            break;
                        }
                        v81 = v83;
                        v82 = v81;
                    }
                    // 0x1a1f9c
                    v50 = v82;
                    v60 = v50 + 1;
                    v84 = v60 - v55; // 0x1a1fa7
                    String::Copy((char *)v76, v55, (int64_t)(v84 + 1));
                    v85 = parms->directorySize; // 0x1a1fd2
                    v86 = v53; // 0x1a1fe0
                    v87 = &v53; // 0x1a1fe0
                    if (v85 >= 1025) {
                        // 0x1a216d
                        v87 = Memory::Allocate((int64_t)v85);
                        v86 = v87;
                    }
                    // 0x1a1ff4
                    v54 = v86;
                    if (::realpath(v77, v87) == 0) {
                        // 0x1a2162
                        v36 = (char *)v50;
                        v42 = v79;
                        v43 = v78;
                        v44 = v77;
                        v57 = v84 + v76;
                        goto lab_0x1a2062;
                    } else {
                        v88 = parms->directorySize;
                        String::Copy((char *)v78, v54, parms->directorySize);
                        v61 = (char *)v50;
                        v89 = *v61; // 0x1a2033
                        if (v89 == 47) {
                            // 0x1a2186
                            File::Path::ForceTrailingSeparator((char *)v79, *v2, 47);
                            goto lab_0x1a204c;
                        } else {
                            if (v89 != 0) {
                                goto lab_0x1a204c;
                            } else {
                                // 0x1a2042
                                if (*(char *)(v50 - 1) == 47) {
                                    // 0x1a2186
                                    File::Path::ForceTrailingSeparator((char *)v79, *v2, 47);
                                    goto lab_0x1a204c;
                                } else {
                                    goto lab_0x1a204c;
                                }
                            }
                        }
                    }
                }
              lab_0x1a1f79:
                // 0x1a1f79
                v72 = *(char *)v52;
                v69 = v46;
                v70 = v47;
                v71 = v45;
                v73 = v52;
                v74 = v51;
                v75 = v58;
                v37 = v59;
                if (v72 == 0) {
                    // break -> 0x1a209e
                    break;
                }
            }
        }
        lab_0x1a209e:
        // 0x1a209e
        String::Copy(v18, v38, parms->directorySize);
        if (v38 != nullptr && v65 != v8) {
            // 0x1a20d5
            Memory::Free(v38);
        }
    }
    String::Copy(parms->directory, v18, parms->directorySize);
    if (basepath != basepathfast && basepath != nullptr) {
        // 0x1a211d
        Memory::Free(basepath);
    }
    if (univpath != univpathfast && univpath != nullptr) {
        // 0x1a2137
        Memory::Free(univpath);
    }

    // Stack fail guard
    return *(char *)*v90 != 0;
  lab_0x1a1e60:
    // 0x1a1e60
    v31 = parms->directorySize;
    if (v28 > v13 != (v29 == v25)) {
        // break -> 0x1a1e71
        goto lab_0x1a1e71_2;
    }
    // 0x1a1eeb
    String::Copy(v30, v23, v31);
    goto lab_0x1a1e71;
  lab_0x1a1e71:;
    char* v93 = String::Find(v27, 47, parms->directorySize); // 0x1a1e89
    v26 = v93;
    if (v93 == 0) {
        // break (via goto) -> 0x1a1f09
        goto lab_0x1a1f09;
    }
    goto lab_0x1a1e94;
  lab_0x1a2062:
    // 0x1a2062
    v58 = v57;
    v45 = v44;
    v47 = v43;
    v46 = v42;
    char v94 = *v36; // 0x1a2062
    v51 = v94 == 0 ? v55 : (char *)v60;
    v52 = v94 == 0 ? v50 : v60;
    if (v54 == nullptr || v53 == v54) {
        // break -> 0x1a1f79
        goto lab_0x1a1f79;
    }
    // 0x1a208b
    Memory::Free((int32_t *)v54);
    v95 = *(char *)v52; // 0x1a2093
    v39 = v46;
    v40 = v47;
    v41 = v45;
    v56 = v58;
    v55 = v51;
    v48 = v95;
    v49 = v52;
    v37 = v59;
    if (v95 == 0) {
        // break (via goto) -> 0x1a209e
        goto lab_0x1a209e;
    }
    goto lab_0x1a1f90;
  lab_0x1a204c:
    // 0x1a204c
    v36 = v61;
    v42 = v59;
    v43 = v59;
    v44 = v59;
    v57 = String::Length(v38) + v59;
    goto lab_0x1a2062;

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
    if (stat(tmp, &sb) != 0) {
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
