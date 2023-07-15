#include "bc/String.hpp"
#include "bc/Debug.hpp"
#include <cstring>
#include <cstdio>
#include <cstdarg>

namespace Blizzard {
namespace String {

int32_t Append(char* buf, const char* append, size_t cap) {
    const char* dx = nullptr;
    int bytesAppended = 0;
    char* outBytes = nullptr;
    char* ceiling = nullptr;
    char inByte = '\0';

    if (!cap || !buf) {
        return 0;
    }

    ceiling = buf + (cap - 1);

    if (buf > ceiling) {
        return 0;
    }

    inByte = *buf;
    outBytes = buf;

    while (inByte != '\0') {
        outBytes = outBytes + 1;
        if (ceiling < outBytes) {
            return outBytes - buf;
        }
        inByte = *outBytes;
    }

    if ((outBytes <= ceiling) && append) {
        if (outBytes < ceiling) {
            inByte = *append;

            while (inByte != '\0') {
                *outBytes = inByte;
                outBytes = outBytes + 1;
                if (ceiling <= outBytes) {
                    break;
                }
                dx     = append + 1;
                append = append + 1;
                inByte = *dx;
            }
        }
        *outBytes = '\0';
    }

    return int32_t(reinterpret_cast<uintptr_t>(outBytes) - reinterpret_cast<uintptr_t>(buf));
}

int32_t Copy(char* dst, const char* src, size_t len) {
    if (!len || !dst) {
        return 0;
    }

    if (!src) {
        *dst = 0;
        return 0;
    }

    char* v3 = dst + len - 1;

    char v4;
    const char* v5;
    char* v6;

    int32_t result;

    if (dst < v3 && (v4 = *src, v5 = src, v6 = dst, *src)) {
        do {
            *v6++ = v4;

            if (v3 <= v6) {
                break;
            }

            v4 = (v5++)[1];
        } while (v4);

        result = v6 - dst;
    } else {
        v6 = dst;
        result = 0;
    }

    *v6 = 0;

    return result;
}

// Find first occurence of char ch within string str
// returns ptr to first occurence, or null if it is not found
char* Find(char* str, char ch, size_t len) {
    // Check if the string is null or empty.
    if (str == nullptr || len == 0) {
        return nullptr;
    }

    auto ptr = str;
    auto end = str + len;

    // Loop through the string, looking for the character.
    while (ptr < end) {
        if (*ptr == '\0') {
            return nullptr;
        }

        if (*ptr == ch) {
            // Return the address of the first occurrence.
            return ptr;
        }

        // Increment the pointer.
        ptr++;
    }

    // The character was not found.
    return nullptr;
}

char* FindFilename(char* str) {
    char  ch     = 0;
    char* result = nullptr;
    auto  ptr    = str;

    if (str == nullptr) {
        return "";
    }

    do {
        do {
            result = ptr;
            ch = *str;
            str = str + 1;
            ptr = str;
        } while (ch == '/');
    } while ((ch == '\\') || (ptr = result, ch != '\0'));
    return result;
}

// Format a string into char* dest, not exceeding uint32_t length.
void Format(char* dst, size_t capacity, const char* format, ...) {
    if (!dst || capacity == 0) {
        return;
    }

    if (!format && capacity >= 1) {
        *dst = '\0';
        return;
    }

    auto formatNative = format;

#if defined(WHOA_SYSTEM_MAC) || defined(WHOA_SYSTEM_LINUX)
    constexpr size_t translatedSize = 2048;

    // POSIX formatting convention requires %ll for 64-bit printing
    // Search-and-replace all instances of %I64 with %ll
    char translated[translatedSize] = {0};

    Translate(format, translated, translatedSize, "%I64", "%ll");

    formatNative = translated;
#endif
    va_list args;
    va_start(args, format);

    vsnprintf(dst, capacity, formatNative, args);

    *dst = '\0';
}

uint32_t Length(const char* str) {
    if (str) {
        return strlen(str);
    }

    return 0;
}

int32_t MemCompare(void* p1, void *p2, size_t len) {
    return memcmp(p1, p2, len);
}

void MemCopy(void* dst, const void* src, size_t len) {
    memmove(dst, src, len);
}

void MemFill(void* dst, uint32_t len, uint8_t fill) {
    memset(dst, fill, len);
}

void Translate(const char* src, char* dest, size_t destSize, char* pattern, char* replacement) {
    if (dest == nullptr || pattern == nullptr || replacement == nullptr) {
        return;
    }

    if (src == nullptr) {
        src = dest;
    }

    // Cap-1 because we need to always affix a null character.
    auto destCeiling = dest + destSize - 1;
    auto srcCeiling  = src + Length(src) + 1;

    auto patternLen     = Length(pattern);
    auto replacementLen = Length(replacement);

    // Current read pointer
    auto srcPtr = src;
    // Current write pointer
    auto destPtr = dest;

    // Process string byte by byte
    // Until dest ceiling is reached
    while (destPtr < destCeiling && srcPtr < srcCeiling) {
        auto byte = *srcPtr;

        if (byte == '\0') {
            break;
        }

        // If this byte is found at the start of pattern
        if (byte == *pattern) {
            // Calculate size of replacement bytes to copy
            // Read size must not read outside of bounds
            size_t copySize = replacementLen;
            if ((destPtr + copySize) >= destCeiling) {
                copySize = destCeiling - destPtr;
            }

            // Look for the rest of the pattern
            auto substring = strstr(srcPtr, pattern);
            // If the read pointer is indeed pointing to an instance of the pattern,
            if (substring == srcPtr) {
                // Copy replacement data instead of original pattern.
                MemCopy(destPtr, replacement, copySize);
                destPtr += replacementLen;
                srcPtr  += patternLen;
                continue;
            }
        }

        // copy a single byte
        *destPtr = *srcPtr;
        srcPtr++;
        destPtr++;
    }

    *destPtr = '\0';
}

void VFormat(char* dst, size_t capacity, const char* format, va_list args) {
    char buffer[0x800] = {0};

    auto formatNative = format;

#if !defined(WHOA_SYSTEM_WIN)
    Translate(format, buffer, 0x800, "%I64", "%ll");
    formatNative = buffer;
#endif

    if (format == nullptr) {
        if (capacity != 0) {
            *dst = '\0';
        }
    } else {
        vsnprintf(dst, capacity, formatNative, args);
        dst[capacity - 1] = '\0';
    }
}

} // namespace String
} // namespace Blizzard
