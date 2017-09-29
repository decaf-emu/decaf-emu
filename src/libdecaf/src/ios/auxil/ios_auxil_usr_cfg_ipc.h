#pragma once
#include "ios_auxil_usr_cfg.h"
#include "ios/kernel/ios_kernel_ipc.h"

namespace ios::auxil
{

using UCHandle = kernel::ResourceHandleId;

Error
UCOpen();

Error
UCClose(UCHandle handle);

UCError
UCReadSysConfig(UCHandle handle,
                uint32_t count,
                phys_ptr<UCSysConfig> settings);

UCError
UCWriteSysConfig(UCHandle handle,
                 uint32_t count,
                 phys_ptr<UCSysConfig> settings);

} // namespace ios::auxil
