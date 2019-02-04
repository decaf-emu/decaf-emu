#include "nn_olv.h"
#include "nn_olv_initializeparam.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/olv/nn_olv_result.h"

using namespace nn::olv;

namespace cafe::nn_olv
{

constexpr auto MinWorkBufferSize = 0x10000u;

virt_ptr<InitializeParam>
InitializeParam_Constructor(virt_ptr<InitializeParam> self)
{
   if (!self) {
      self = virt_cast<InitializeParam *>(ghs::malloc(sizeof(InitializeParam)));
      if (!self) {
         return nullptr;
      }
   }

   self->flags = 0u;
   self->reportTypes = 0x1B7Fu;
   self->workBuffer = nullptr;
   self->workBufferSize = 0u;
   self->sysArgs = nullptr;
   self->sysArgsSize = 0u;
   return self;
}

nn::Result
InitializeParam_SetFlags(virt_ptr<InitializeParam> self,
                         uint32_t flags)
{
   self->flags = flags;
   return ResultSuccess;
}

nn::Result
InitializeParam_SetWork(virt_ptr<InitializeParam> self,
                        virt_ptr<uint8_t> workBuffer,
                        uint32_t workBufferSize)
{
   if (!workBuffer) {
      return ResultInvalidPointer;
   }

   if (workBufferSize < MinWorkBufferSize) {
      return ResultInvalidSize;
   }

   self->workBuffer = workBuffer;
   self->workBufferSize = workBufferSize;
   return ResultSuccess;
}

nn::Result
InitializeParam_SetReportTypes(virt_ptr<InitializeParam> self,
                               uint32_t types)
{
   self->reportTypes = types;
   return ResultSuccess;
}

nn::Result
InitializeParam_SetSysArgs(virt_ptr<InitializeParam> self,
                           virt_ptr<uint8_t> sysArgs,
                           uint32_t sysArgsSize)
{
   if (!sysArgs) {
      return ResultInvalidPointer;
   }

   if (!sysArgsSize) {
      return ResultInvalidSize;
   }

   self->sysArgs = sysArgs;
   self->sysArgsSize = sysArgsSize;
   return ResultSuccess;
}

void
Library::registerInitializeParamSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn3olv15InitializeParamFv",
                              InitializeParam_Constructor);
   RegisterFunctionExportName("SetFlags__Q3_2nn3olv15InitializeParamFUi",
                              InitializeParam_SetFlags);
   RegisterFunctionExportName("SetWork__Q3_2nn3olv15InitializeParamFPUcUi",
                              InitializeParam_SetWork);
   RegisterFunctionExportName("SetReportTypes__Q3_2nn3olv15InitializeParamFUi",
                              InitializeParam_SetReportTypes);
   RegisterFunctionExportName("SetSysArgs__Q3_2nn3olv15InitializeParamFPCvUi",
                              InitializeParam_SetSysArgs);
}

}  // namespace cafe::nn_olv
