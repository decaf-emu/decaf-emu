include_directories(".")

set(SOURCE_FILES
    )

set(HEADER_FILES
    platform_dir.h
    platform_exception.h
    platform_fiber.h
    platform.h
    platform_memorymap.h
    platform_thread.h
    platform_time.h
    )

set(SOURCE_FILES_WINDOWS
    platform_win_dir.cpp
    platform_win_exception.cpp
    platform_win_fiber.cpp
    platform_win_memorymap.cpp
    platform_win_thread.cpp
    platform_win_time.cpp
    )
set(HEADER_FILES_WINDOWS
    )

set(SOURCE_FILES_POSIX
    platform_posix_dir.cpp
    platform_posix_exception.cpp
    platform_posix_fiber.cpp
    platform_posix_memorymap.cpp
    platform_posix_thread.cpp
    platform_posix_time.cpp
    )
set(HEADER_FILES_POSIX
    )

add_library(platform STATIC ${SOURCE_FILES} ${HEADER_FILES} ${SOURCE_FILES_POSIX} ${HEADER_FILES_POSIX})
