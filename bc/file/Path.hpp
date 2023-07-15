#ifndef BC_FILE_PATH_HPP
#define BC_FILE_PATH_HPP

#include "bc/file/Defines.hpp"

namespace Blizzard {
namespace File {
namespace Path {

// Classes

// Quick native path translation on the stack.
// Can be slow in some instances, as it resorts to heap allocation if path size >= BC_FILE_MAX_PATH
// Note: this name is an invention, however its behavior is repeated in all System_File implementations.
// So it probably did exist as an inlined class.
class QuickNative {
    private:
        size_t size;
        // stack allocation
        char fastpath[BC_FILE_MAX_PATH];
        // heap
        char* fatpath;

    public:
        QuickNative(const char* path);
        ~QuickNative();
        const char* Str();
        size_t Size();
};

// Functions

void ForceTrailingSeparator(char* buf, size_t bufMax, char sep);

bool MakeBackslashPath(const char* path, char* result, size_t capacity);

bool MakeConsistentPath(const char* path, char* result, size_t capacity);

bool MakeNativePath(const char* path, char* result, size_t capacity);

bool MakeUnivPath(const char* path, char* result, size_t capacity);

bool MakeWindowsPath(const char* path, char* result, size_t capacity);

} // namespace Path
} // namespace File
} // namespace Blizzard

#endif
