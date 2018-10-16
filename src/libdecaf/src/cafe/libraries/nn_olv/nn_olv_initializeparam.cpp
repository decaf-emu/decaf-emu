#include "nn_olv.h"
#include "nn_olv_initializeparam.h"

#include "nn/olv/nn_olv_result.h"

using namespace nn::olv;

namespace cafe::nn_olv
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

nn::Result
InitializeParam::SetFlags(uint32_t flags)
{
   mFlags = flags;
   return ResultSuccess;
}

nn::Result
InitializeParam::SetWork(virt_ptr<uint8_t> workBuffer,
                         uint32_t workBufferSize)
{
   if (!workBuffer) {
      return ResultInvalidPointer;
   }

   if (workBufferSize < MinWorkBufferSize) {
      return ResultInvalidSize;
   }

   mWorkBuffer = workBuffer;
   mWorkBufferSize = workBufferSize;
   return ResultSuccess;
}

nn::Result
InitializeParam::SetReportTypes(uint32_t types)
{
   mReportTypes = types;
   return ResultSuccess;
}

nn::Result
InitializeParam::SetSysArgs(virt_ptr<uint8_t> sysArgs,
                            uint32_t sysArgsSize)
{
   if (!sysArgs) {
      return ResultInvalidPointer;
   }

   if (!sysArgsSize) {
      return ResultInvalidSize;
   }

   mSysArgs = sysArgs;
   mSysArgsSize = sysArgsSize;
   return ResultSuccess;
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

}  // namespace cafe::nn_olv
