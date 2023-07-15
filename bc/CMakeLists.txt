file(GLOB BC_SOURCES
    "*.cpp"
    "lock/*.cpp"
    "file/*.cpp"
    "time/*.cpp"
    "file/system/*.cpp"
    "time/system/*.cpp"
)

if(DEFINED WHOA_SYSTEM_WIN)
    file(GLOB BC_FILE_SYSTEM_SOURCES "file/system/win/*.cpp")
endif()

if(DEFINED WHOA_SYSTEM_LINUX OR WHOA_SYSTEM_MAC)
    file(GLOB BC_FILE_SYSTEM_SOURCES "file/system/posix/*.cpp")
endif()

list(APPEND BC_SOURCES ${BC_FILE_SYSTEM_SOURCES})

add_library(bc STATIC
    ${BC_SOURCES}
)

target_include_directories(bc
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(bc
    PUBLIC
        storm
)
