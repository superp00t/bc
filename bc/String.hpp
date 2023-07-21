#ifndef BC_STRING_HPP
#define BC_STRING_HPP

#include <cstdint>
#include <cstdlib>
#include <cstdarg>

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

        QuickFormat(const char* format, ...);
        const char* Str();
};

// Functions
int32_t Append(char* dst, const char* src, size_t cap);

int32_t Copy(char* dst, const char* src, size_t len);

char* Find(char* str, char ch, size_t len);

const char* FindFilename(const char* str);

void Format(char* dest, size_t capacity, const char* format, ...);

uint32_t Length(const char* str);

void MemFill(void* dst, uint32_t len, uint8_t fill);

void MemCopy(void* dst, const void* src, size_t len);

int32_t MemCompare(void* p1, void *p2, size_t len);

void Translate(const char* src, char* dest, size_t destSize, const char* pattern, const char* replacement);

void VFormat(char* dst, size_t capacity, const char* format, va_list args);

template <size_t Cap>
QuickFormat<Cap>::QuickFormat(const char* format, ...) {
    va_list args;
    va_start(args, format);
    VFormat(this->buffer, Cap, format, args);
}

template <size_t Cap>
const char* QuickFormat<Cap>::Str() {
    return static_cast<const char*>(this->buffer);
}

} // namespace String
} // namespace Blizzard

#endif
