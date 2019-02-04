#pragma once
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

struct InitializeParam
{
   be2_val<uint32_t> flags;
   be2_val<uint32_t> reportTypes;
   be2_virt_ptr<uint8_t> workBuffer;
   be2_val<uint32_t> workBufferSize;
   be2_virt_ptr<uint8_t> sysArgs;
   be2_val<uint32_t> sysArgsSize;
   UNKNOWN(0x28);
};
CHECK_OFFSET(InitializeParam, 0x00, flags);
CHECK_OFFSET(InitializeParam, 0x04, reportTypes);
CHECK_OFFSET(InitializeParam, 0x08, workBuffer);
CHECK_OFFSET(InitializeParam, 0x0C, workBufferSize);
CHECK_OFFSET(InitializeParam, 0x10, sysArgs);
CHECK_OFFSET(InitializeParam, 0x14, sysArgsSize);
CHECK_SIZE(InitializeParam, 0x40);

virt_ptr<InitializeParam>
InitializeParam_Constructor(virt_ptr<InitializeParam> self);

nn::Result
InitializeParam_SetFlags(virt_ptr<InitializeParam> self,
                         uint32_t flags);

nn::Result
InitializeParam_SetWork(virt_ptr<InitializeParam> self,
                        virt_ptr<uint8_t> workBuffer,
                        uint32_t workBufferSize);

nn::Result
InitializeParam_SetReportTypes(virt_ptr<InitializeParam> self,
                               uint32_t types);

nn::Result
InitializeParam_SetSysArgs(virt_ptr<InitializeParam> self,
                           virt_ptr<uint8_t> sysArgs,
                           uint32_t sysArgsSize);

}  // namespace cafe::nn_olv
