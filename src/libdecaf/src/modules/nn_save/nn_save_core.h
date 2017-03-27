#pragma once
#include "filesystem/filesystem_path.h"
#include "modules/coreinit/coreinit_fs.h"

namespace nn
{

namespace save
{

using SaveStatus = coreinit::FSStatus;

SaveStatus
SAVEInit();

void
SAVEShutdown();

SaveStatus
SAVEInitSaveDir(uint8_t userID);

SaveStatus
SAVEGetSharedDataTitlePath(uint64_t titleID,
                           const char *dir,
                           char *buffer,
                           uint32_t bufferSize);

SaveStatus
SAVEGetSharedSaveDataPath(uint64_t titleID,
                          const char *dir,
                          char *buffer,
                          uint32_t bufferSize);

namespace internal
{

fs::Path
getSaveDirectory(uint32_t account);

fs::Path
getSavePath(uint32_t account,
            const char *path);

} // namespace internal

} // namespace save

} // namespace nn
