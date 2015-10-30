include_directories(".")

set(SOURCE_FILES
	)

set(HEADER_FILES
	platform.h
	platform_thread.h
	platform_time.h
	platform_ui.h
	)

set(SOURCE_FILES_WINDOWS
	platform_win_thread.cpp
	platform_win_time.cpp
	platform_win_ui.cpp
    )
set(HEADER_FILES_WINDOWS
    )

set(SOURCE_FILES_POSIX
	platform_posix_thread.cpp
	platform_posix_time.cpp
	platform_posix_ui.cpp
    )
set(HEADER_FILES_POSIX
    )

add_library(platform STATIC ${SOURCE_FILES} ${HEADER_FILES} ${SOURCE_FILES_POSIX} ${HEADER_FILES_POSIX})
