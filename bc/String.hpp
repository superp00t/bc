#ifndef BC_STRING_HPP
#define BC_STRING_HPP

#include <cstdint>
#include <cstdlib>
#include <cstdarg>

#define BC_STRING_FORMAT_SIZE 2048

#if defined(WHOA_SYSTEM_WIN)
#define BC_FILE_SYSTEM_PATH_SEPARATOR '\\'
#else
#define BC_FILE_SYSTEM_PATH_SEPARATOR '/'
#endif

namespace Blizzard {
namespace String {

// Types

template<size_t Cap>
class QuickFormat {
    public:
        char buffer[Cap];

        QuickFormat(char* format, ...) {
            va_list args;
            va_start(args, format);
            VFormat(this->buffer, Cap, format, args);
        }

        const char* Str() {
            return static_cast<const char*>(this->buffer);
        }
};

// Functions
int32_t Append(char* dst, const char* src, size_t cap);

int32_t Copy(char* dst, const char* src, size_t len);

char* Find(char* str, char ch, size_t len);

char* FindFilename(char* str);

void Format(char* dest, size_t capacity, const char* format, ...);

uint32_t Length(const char* str);

void MemFill(void* dst, uint32_t len, uint8_t fill);

void MemCopy(void* dst, const void* src, size_t len);

int32_t MemCompare(void* p1, void *p2, size_t len);

void Translate(const char* src, char* dest, size_t destSize, char* pattern, char* replacement);

void VFormat(char* dst, size_t capacity, const char* format, va_list args);

} // namespace String
} // namespace Blizzard

#endif
