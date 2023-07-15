#include "bc/file/Filesystem.hpp"
#include <cstddef>

namespace Blizzard {
namespace File {

bool Filesystem::Do(Filesystem::Call fscall, System_File::Stacked::FileParms* parms) {
    uint32_t callindex = static_cast<uint32_t>(fscall);
    // Mark byte offset of Call function.
    // Allows the filesystem stack to resume normal activity in case of a fallback
    parms->offset = offsetof(Filesystem, funcs) + (callindex * sizeof(uintptr_t));
    // Get filesystem call from array

    auto func = reinterpret_cast<Filesystem::CallFunc>(this->funcs[callindex]);
    // Do call
    return func(this, parms);
}

} // namespace File
} // namespace Blizzard

