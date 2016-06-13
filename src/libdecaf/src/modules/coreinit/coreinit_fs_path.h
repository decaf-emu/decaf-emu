#pragma once
#include "filesystem/filesystem.h"

namespace coreinit
{

/**
 * \ingroup coreinit_fs
 */

FSStatus
FSGetCwd(FSClient *client,
         FSCmdBlock *block,
         char *buffer,
         uint32_t bufferSize,
         uint32_t flags);

FSStatus
FSChangeDir(FSClient *client,
            FSCmdBlock *block,
            const char *path,
            uint32_t flags);

FSStatus
FSChangeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 const char *path,
                 uint32_t flags,
                 FSAsyncData *asyncData);

/** @} */

namespace internal
{

fs::Path
translatePath(const char *path);

} // namespace internal

} // namespace coreinit
