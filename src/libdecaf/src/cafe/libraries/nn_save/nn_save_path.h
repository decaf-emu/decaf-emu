#pragma once
#include "cafe/libraries/coreinit/coreinit_fs.h"
#include "filesystem/filesystem_path.h"
#include "nn/nn_result.h"
#include "nn/act/nn_act_types.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::nn_save
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
                           virt_ptr<const char> dir,
                           virt_ptr<char> buffer,
                           uint32_t bufferSize);

SaveStatus
SAVEGetSharedSaveDataPath(uint64_t titleID,
                          virt_ptr<const char> dir,
                          virt_ptr<char> buffer,
                          uint32_t bufferSize);

namespace internal
{

SaveStatus
translateResult(nn::Result result);

bool
getPersistentId(nn::act::SlotNo slot,
                uint32_t &outPersistentId);

fs::Path
getSaveDirectory(nn::act::SlotNo slot);

fs::Path
getSavePath(nn::act::SlotNo slot,
            std::string_view path);

fs::Path
getTitleSaveDirectory(uint64_t title,
                      nn::act::SlotNo slot);

fs::Path
getTitleSavePath(uint64_t title,
                 nn::act::SlotNo slot,
                 std::string_view path);

} // namespace internal

} // namespace cafe::nn_save
