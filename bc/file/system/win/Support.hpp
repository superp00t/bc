#ifndef BC_FILE_SYSTEM_WIN_SUPPORT_HPP
#define BC_FILE_SYSTEM_WIN_SUPPORT_HPP

// Lowercase windows.h macros fight with BlizzardCore file function names
// Get out of here
#if defined(WHOA_SYSTEM_WIN)
#if defined(GetFreeSpace)
#undef GetFreeSpace
#endif
#endif

#endif
