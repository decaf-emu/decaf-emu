#include "nn_olv.h"
#include "nn_olv_initializeparam.h"
#include "nn_olv_result.h"

namespace cafe::nn::olv
{

constexpr auto MinWorkBufferSize = 0x10000u;

InitializeParam::InitializeParam()
{
   mFlags = 0u;
   mReportTypes = 0x1B7Fu;
   mWorkBuffer = nullptr;
   mWorkBufferSize = 0u;
   mSysArgs = nullptr;
   mSysArgsSize = 0u;
}

Result
InitializeParam::SetFlags(uint32_t flags)
{
   mFlags = flags;
   return Success;
}

Result
InitializeParam::SetWork(virt_ptr<uint8_t> workBuffer,
                         uint32_t workBufferSize)
{
   if (!workBuffer) {
      return InvalidPointer;
   }

   if (workBufferSize < MinWorkBufferSize) {
      return InvalidSize;
   }

   mWorkBuffer = workBuffer;
   mWorkBufferSize = workBufferSize;
   return Success;
}

Result
InitializeParam::SetReportTypes(uint32_t types)
{
   mReportTypes = types;
   return Success;
}

Result
InitializeParam::SetSysArgs(virt_ptr<uint8_t> sysArgs,
                            uint32_t sysArgsSize)
{
   if (!sysArgs) {
      return InvalidPointer;
   }

   if (!sysArgsSize) {
      return InvalidSize;
   }

   mSysArgs = sysArgs;
   mSysArgsSize = sysArgsSize;
   return Success;
}

void
Library::registerInitializeParamSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn3olv15InitializeParamFv",
                             InitializeParam);
   RegisterFunctionExportName("SetFlags__Q3_2nn3olv15InitializeParamFUi",
                              &InitializeParam::SetFlags);
   RegisterFunctionExportName("SetWork__Q3_2nn3olv15InitializeParamFPUcUi",
                              &InitializeParam::SetWork);
   RegisterFunctionExportName("SetReportTypes__Q3_2nn3olv15InitializeParamFUi",
                              &InitializeParam::SetReportTypes);
   RegisterFunctionExportName("SetSysArgs__Q3_2nn3olv15InitializeParamFPCvUi",
                              &InitializeParam::SetSysArgs);
}

}  // namespace cafe::nn::olv
