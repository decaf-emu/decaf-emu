#pragma once
#include "coreinit_fs.h"

namespace coreinit
{

/**
 * \ingroup coreinit_fs
 */

FSStatus
FSGetStat(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          FSStat *stat,
          uint32_t flags);

FSStatus
FSGetStatAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               FSStat *stat,
               uint32_t flags,
               FSAsyncData *asyncData);

FSStatus
FSGetStatFile(FSClient *client,
              FSCmdBlock *block,
              FSFileHandle handle,
              FSStat *stat,
              uint32_t flags);

FSStatus
FSGetStatFileAsync(FSClient *client,
                   FSCmdBlock *block,
                   FSFileHandle handle,
                   FSStat *stat,
                   uint32_t flags,
                   FSAsyncData *asyncData);

/** @} */

} // namespace coreinit
