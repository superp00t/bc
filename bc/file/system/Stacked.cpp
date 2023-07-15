
#include "bc/file/Filesystem.hpp"
#include "bc/file/system/System_File.hpp"
#include "bc/file/system/Stacked.hpp"
#include "bc/String.hpp"
#include "bc/Memory.hpp"

namespace Blizzard {
namespace System_File {
namespace Stacked {

bool null_cd(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_close(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_create(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_cwd(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_dirwalk(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_exists(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_flush(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_getfileinfo(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_getfreespace(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_getpos(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_getrootchars(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_isabspath(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_isreadonly(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_makeabspath(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_mkdir(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_move(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_copy(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_open(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_read(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_readp(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_rmdir(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_setcachemode(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_seteof(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_setfileinfo(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_setpos(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_unlink(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_write(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_writep(File::Filesystem* fs, FileParms* parms) {
    return false;
}
bool null_shutdown(File::Filesystem* fs, FileParms* parms) {
    return false;
}

// Null stack: this stack does absolutely nothing. Its only purpose is to store fallback functions.
static File::Filesystem s_nullstack = {
    &s_nullstack,

    nullptr,

    {
		null_cd,
		null_close,
		null_create,
		null_cwd,
		null_dirwalk,
		null_exists,
		null_flush,
		null_getfileinfo,
		null_getfreespace,
		null_getpos,
		null_getrootchars,
		null_isabspath,
		null_isreadonly,
		null_makeabspath,
		null_mkdir,
		null_move,
		null_copy,
		null_open,
		null_read,
		null_readp,
		null_rmdir,
		null_setcachemode,
		null_seteof,
		null_setfileinfo,
		null_setpos,
		null_unlink,
		null_write,
		null_writep,
		null_shutdown
    }
};

// Base stack: this stack contains the base System_File functions.
static File::Filesystem s_basestack = {
    &s_basestack,

    nullptr,

    {
        System_File::SetWorkingDirectory,
        System_File::Close,
        System_File::Create,
        System_File::GetWorkingDirectory,
        System_File::ProcessDirFast,
        System_File::Exists,
        System_File::Flush,
        System_File::GetFileInfo,
        System_File::GetFreeSpace,
        System_File::GetPos,
        System_File::GetRootChars,
        System_File::IsAbsolutePath,
        System_File::IsReadOnly,
        System_File::MakeAbsolutePath,
        System_File::CreateDirectory,
        System_File::Move,
        System_File::Copy,
        System_File::Open,
        System_File::Read,
        System_File::ReadP,
        System_File::RemoveDirectory,
        System_File::SetCacheMode,
        System_File::SetEOF,
        System_File::SetAttributes,
        System_File::SetPos,
        System_File::Delete,
        System_File::Write,
        System_File::WriteP,
        System_File::Shutdown
    }
};

// File initialization stack: this is the default
static File::Filesystem s_fileinit = {
    &s_fileinit,

    nullptr,

    {
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init,
        file_init
    }
};

// Manager will be initialized upon first use
static File::Filesystem* s_manager = &s_fileinit;

// Manager getter
File::Filesystem* Manager() {
    return s_manager;
}

// Push a filesystem stack to manager linked list
void Push(File::Filesystem* fs) {
    auto stack = reinterpret_cast<File::Filesystem*>(Memory::Allocate(sizeof(File::Filesystem)));
    String::MemFill(stack, sizeof(File::Filesystem), 0);

    // Add stack
    String::MemCopy(stack, fs, sizeof(File::Filesystem));
    stack->next = s_manager;

    s_manager = stack;

    HoistAll();
}

// file_init is part of the default filesystem stack
// The first use of s_manager will call this function only once; it is a fallback function in case s_basestack's funcs are not yet hoisted.
bool file_init(File::Filesystem* fs, FileParms* parms) {
    // Allocate a null stack, replacing what was previously a pointer to s_fileinit
    s_manager = reinterpret_cast<File::Filesystem*>(Memory::Allocate(sizeof(File::Filesystem)));
    if (s_manager == nullptr) {
        return false;
    }

    // Add null fs calls to stack
    // After Push/HoistAll, if a filesystem call is unimplemented, the func from s_nullstack gets called instead.
    String::MemCopy(s_manager, &s_nullstack, sizeof(File::Filesystem));

    // Hoist basic file functions
    Push(&s_basestack);

    // fs manager is now ready to use!
    // Execute the original call.
    if (s_manager != nullptr) {
        auto func = *reinterpret_cast<File::Filesystem::CallFunc*>(reinterpret_cast<uintptr_t>(s_manager) + parms->offset);
        return func(fs, parms);
    }

    return false;
}

// Ensures that the filesystem manager will always have a corresponding function address for enum Call call.
void Hoist(uint32_t call) {
    if (s_manager->funcs[call] == nullptr) {
        auto topStack = s_manager;
        auto prev = s_manager;
        auto next = s_manager->next;
        while (next && topStack->funcs[call] == nullptr) {
            topStack->funcs[call] = next->funcs[call];
            prev = next;
            next = next->next;
        }

        // Remove call from lower stack
        prev->funcs[call] = nullptr;
    }
}

// Hoist all functions in the filesystem stack to the top, i.e. the manager
void HoistAll() {
    for (uint32_t call = 0; call < File::Filesystem::s_numCalls; call++) {
        Hoist(call);
    }
}

} // namespace Stacked
} // namespace System_File
} // namespace Blizzard
