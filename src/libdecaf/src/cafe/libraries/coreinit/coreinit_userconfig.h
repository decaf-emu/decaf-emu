#pragma once
#include "coreinit_enum.h"
#include "coreinit_ios.h"
#include "ios/auxil/ios_auxil_usr_cfg.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_userconfig User Config
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

using ios::auxil::UCCommand;
using ios::auxil::UCDataType;
using ios::auxil::UCError;

// This is a copy of ios::auxil::UCSysConfig but with a virt_ptr
struct UCSysConfig
{
   be2_array<char, 64> name;
   be2_val<uint32_t> access;
   be2_val<UCDataType> dataType;
   be2_val<UCError> error;
   be2_val<uint32_t> dataSize;
   be2_virt_ptr<void> data;
};
CHECK_OFFSET(UCSysConfig, 0x00, name);
CHECK_OFFSET(UCSysConfig, 0x40, access);
CHECK_OFFSET(UCSysConfig, 0x44, dataType);
CHECK_OFFSET(UCSysConfig, 0x48, error);
CHECK_OFFSET(UCSysConfig, 0x4C, dataSize);
CHECK_OFFSET(UCSysConfig, 0x50, data);
CHECK_SIZE(UCSysConfig, 0x54);

using UCAsyncCallbackFn = virt_func_ptr<void(UCError result,
                                             UCCommand command,
                                             uint32_t count,
                                             virt_ptr<UCSysConfig> settings,
                                             virt_ptr<void> context)>;

struct UCAsyncParams
{
   be2_val<UCAsyncCallbackFn> callback;
   be2_virt_ptr<void> context;
   be2_val<UCCommand> command;
   be2_val<uint32_t> unk0x0C;
   be2_val<uint32_t> count;
   be2_virt_ptr<UCSysConfig> settings;
   be2_virt_ptr<IOSVec> vecs;
};
CHECK_OFFSET(UCAsyncParams, 0x00, callback);
CHECK_OFFSET(UCAsyncParams, 0x04, context);
CHECK_OFFSET(UCAsyncParams, 0x08, command);
CHECK_OFFSET(UCAsyncParams, 0x0C, unk0x0C);
CHECK_OFFSET(UCAsyncParams, 0x10, count);
CHECK_OFFSET(UCAsyncParams, 0x14, settings);
CHECK_OFFSET(UCAsyncParams, 0x18, vecs);
CHECK_SIZE(UCAsyncParams, 0x1C);

#pragma pack(pop)

UCError
UCOpen();

UCError
UCClose(IOSHandle handle);

UCError
UCDeleteSysConfig(IOSHandle handle,
                  uint32_t count,
                  virt_ptr<UCSysConfig> settings);

UCError
UCDeleteSysConfigAsync(IOSHandle handle,
                       uint32_t count,
                       virt_ptr<UCSysConfig> settings,
                       virt_ptr<UCAsyncParams> asyncParams);

UCError
UCReadSysConfig(IOSHandle handle,
                uint32_t count,
                virt_ptr<UCSysConfig> settings);

UCError
UCReadSysConfigAsync(IOSHandle handle,
                     uint32_t count,
                     virt_ptr<UCSysConfig> settings,
                     virt_ptr<UCAsyncParams> asyncParams);

UCError
UCWriteSysConfig(IOSHandle handle,
                 uint32_t count,
                 virt_ptr<UCSysConfig> settings);

UCError
UCWriteSysConfigAsync(IOSHandle handle,
                      uint32_t count,
                      virt_ptr<UCSysConfig> settings,
                      virt_ptr<UCAsyncParams> asyncParams);

/** @} */

} // namespace cafe::coreinit
