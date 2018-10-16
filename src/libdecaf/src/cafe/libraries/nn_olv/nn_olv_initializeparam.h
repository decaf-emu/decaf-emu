#pragma once
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_olv
{

class InitializeParam
{
public:
   InitializeParam();

   nn::Result
   SetFlags(uint32_t flags);

   nn::Result
   SetWork(virt_ptr<uint8_t> workBuffer,
           uint32_t workBufferSize);

   nn::Result
   SetReportTypes(uint32_t types);

   nn::Result
   SetSysArgs(virt_ptr<uint8_t> sysArgs,
              uint32_t sysArgsSize);

protected:
   be2_val<uint32_t> mFlags;
   be2_val<uint32_t> mReportTypes;
   be2_virt_ptr<uint8_t> mWorkBuffer;
   be2_val<uint32_t> mWorkBufferSize;
   be2_virt_ptr<uint8_t> mSysArgs;
   be2_val<uint32_t> mSysArgsSize;
   UNKNOWN(0x28);

private:
   CHECK_MEMBER_OFFSET_BEG
   CHECK_OFFSET(InitializeParam, 0x00, mFlags);
   CHECK_OFFSET(InitializeParam, 0x04, mReportTypes);
   CHECK_OFFSET(InitializeParam, 0x08, mWorkBuffer);
   CHECK_OFFSET(InitializeParam, 0x0C, mWorkBufferSize);
   CHECK_OFFSET(InitializeParam, 0x10, mSysArgs);
   CHECK_OFFSET(InitializeParam, 0x14, mSysArgsSize);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(InitializeParam, 0x40);

}  // namespace cafe::nn_olv
