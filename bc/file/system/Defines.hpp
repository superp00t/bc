#ifndef BC_FILE_SYSTEM_DEFINES_HPP
#define BC_FILE_SYSTEM_DEFINES_HPP

// Constants

// How much data to buffer when copying with File::Copy
#define BC_FILE_SYSTEM_COPYBUFFER_SIZE 0xA00000UL

// Utility macros

#define BC_FILE_PATH(localname) char localname[BC_FILE_MAX_PATH] = {0}

#endif
