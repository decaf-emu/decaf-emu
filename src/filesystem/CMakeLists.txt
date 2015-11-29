include_directories(".")

set(SOURCE_FILES
    )
set(HEADER_FILES
    filesystem_file.h
    filesystem_filehandle.h
    filesystem_folder.h
    filesystem_folderhandle.h
    filesystem.h
    filesystem_host_file.h
    filesystem_host_filehandle.h
    filesystem_host_folder.h
    filesystem_host_folderhandle.h
    filesystem_host_path.h
    filesystem_node.h
    filesystem_path.h
    filesystem_virtual_file.h
    filesystem_virtual_folder.h
    filesystem_virtual_folderhandle.h
    )

set(SOURCE_FILES_WINDOWS
    filesystem_win_host_folder.cpp
    filesystem_win_host_folderhandle.cpp
    )
set(HEADER_FILES_WINDOWS
    )

set(SOURCE_FILES_POSIX
    filesystem_posix_host_folder.cpp
    filesystem_posix_host_folderhandle.cpp
    )
set(HEADER_FILES_POSIX
    )

add_library(filesystem STATIC ${SOURCE_FILES} ${HEADER_FILES} ${SOURCE_FILES_POSIX} ${HEADER_FILES_POSIX})
