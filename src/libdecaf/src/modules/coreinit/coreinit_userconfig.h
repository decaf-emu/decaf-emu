#pragma once
#include "coreinit_enum.h"
#include "coreinit_ios.h"
#include "ios/auxil/ios_auxil_usr_cfg.h"
#include "ppcutils/wfunc_ptr.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

namespace coreinit
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
using ios::auxil::UCSysConfig;

using UCAsyncCallback = wfunc_ptr<void,
                                  UCError /* result */,
                                  UCCommand /* command */,
                                  uint32_t /* count */,
                                  UCSysConfig * /* settings */,
                                  void * /* context */>;

struct UCAsyncParams
{
   UCAsyncCallback::be callback;
   be_ptr<void> context;
   be_val<UCCommand> command;
   be_val<uint32_t> unk0x0C;
   be_val<uint32_t> count;
   be_ptr<UCSysConfig> settings;
   be_ptr<IOSVec> vecs;
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
                  UCSysConfig *settings);

UCError
UCReadSysConfig(IOSHandle handle,
                uint32_t count,
                UCSysConfig *settings);

UCError
UCReadSysConfigAsync(IOSHandle handle,
                     uint32_t count,
                     UCSysConfig *settings,
                     UCAsyncParams *asyncParams);

UCError
UCWriteSysConfig(IOSHandle handle,
                 uint32_t count,
                 UCSysConfig *settings);

UCError
UCWriteSysConfigAsync(IOSHandle handle,
                      uint32_t count,
                      UCSysConfig *settings,
                      UCAsyncParams *asyncParams);

/** @} */

} // namespace coreinit
