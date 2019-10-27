#include "nn_sl.h"
#include "nn_sl_lib.h"
#include "nn_sl_drctransferrer.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/coreinit/coreinit_cosreport.h"

namespace cafe::nn_sl
{

struct StaticLibData
{
   StaticLibData()
   {
      DrcTransferrer_Constructor(virt_addrof(drcTransferrer));
   }

   be2_val<BOOL> initialised;
   be2_struct<DrcTransferrer> drcTransferrer;

   be2_val<AllocFn> allocFn;
   be2_val<FreeFn> freeFn;
   be2_virt_ptr<ITransferrer> transferrer;
};

static virt_ptr<StaticLibData>
sLibData = nullptr;

nn::Result
Initialize(AllocFn allocFn,
           FreeFn freeFn)
{
   return Initialize(allocFn, freeFn, virt_cast<ITransferrer *>(GetDrcTransferrer()));
}

nn::Result
Initialize(AllocFn allocFn,
           FreeFn freeFn,
           virt_ptr<ITransferrer> transferrer)
{
   if (!sLibData->initialised) {
      sLibData->allocFn = allocFn;
      sLibData->freeFn = freeFn;
      sLibData->transferrer = transferrer;
      if (!transferrer) {
         coreinit::internal::COSWarn(coreinit::COSReportModule::Unknown1,
                                     "NULL transferrer is set!");
      }

      // nn::sl::FileSystemAccessor::Initialize(nn::sl::GetFileSystemAccessor())
      sLibData->initialised = TRUE;
   }

   return nn::ResultSuccess;
}

nn::Result
Finalize()
{
   if (sLibData->initialised) {
      // nn::sl::FileSystemAccessor::Finalize(nn::sl::GetFileSystemAccessor())
      sLibData->initialised = FALSE;
   }

   return nn::ResultSuccess;
}

virt_ptr<DrcTransferrer>
GetDrcTransferrer()
{
   return virt_addrof(sLibData->drcTransferrer);
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExportName(
      "Initialize__Q2_2nn2slFPFUiT1_PvPFPv_v",
      static_cast<nn::Result(*)(AllocFn,FreeFn)>(Initialize));
   RegisterFunctionExportName(
      "Initialize__Q4_2nn2sl4core6detailFPFUiT1_PvPFPv_vRQ3_2nn2sl12ITransferrer",
      static_cast<nn::Result(*)(AllocFn, FreeFn, virt_ptr<ITransferrer>)>(Initialize));
   RegisterFunctionExportName("Finalize__Q2_2nn2slFv", Finalize);
   RegisterFunctionExportName("GetDrcTransferrer__Q2_2nn2slFv", GetDrcTransferrer);

   RegisterDataInternal(sLibData);
}

}  // namespace namespace cafe::nn_sl
