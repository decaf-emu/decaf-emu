#include "nn_cmpt.h"
#include "nn_cmpt_lib.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_userconfig.h"

using namespace cafe::coreinit;

namespace cafe::nn_cmpt
{

CMPTError
CMPTGetDataSize(virt_ptr<uint32_t> outDataSize)
{
   *outDataSize = 12u * 1024u * 1024u;
   return CMPTError::OK;
}

CMPTError
CMPTAcctGetPcConf(virt_ptr<CMPTPcConf> outConf)
{
   StackObject<uint32_t> rating { 0 };
   StackObject<uint32_t> organisation { 0 };
   StackObject<uint8_t> rst_internet_ch { 0 };
   StackObject<uint8_t> rst_nw_access { 0 };
   StackObject<uint8_t> rst_pt_order { 0 };
   StackArray<UCSysConfig, 5> config { {
      { "wii_acct.pc.rating", 0u, UCDataType::UnsignedInt, UCError::OK, 4u, rating },
      { "wii_acct.pc.organization", 0u, UCDataType::UnsignedInt, UCError::OK, 4u, organisation },
      { "wii_acct.pc.rst_internet_ch", 0u, UCDataType::UnsignedByte, UCError::OK, 1u, rst_internet_ch },
      { "wii_acct.pc.rst_nw_access", 0u, UCDataType::UnsignedByte, UCError::OK, 1u, rst_nw_access },
      { "wii_acct.pc.rst_pt_order", 0u, UCDataType::UnsignedByte, UCError::OK, 1u, rst_pt_order },
   } };

   auto error = UCOpen();
   if (error < 0) {
      return CMPTError::UserConfigError;
   }

   auto handle = static_cast<IOSHandle>(error);
   error = UCReadSysConfig(handle, 5, config);
   UCClose(handle);

   if (error != UCError::OK) {
      return CMPTError::UserConfigError;
   }

   outConf->rating = *rating;
   outConf->organisation = *organisation;
   outConf->flags = CMPTPcConfFlags::None;

   if (*rst_internet_ch) {
      outConf->flags = CMPTPcConfFlags::RstInternetCh;
   }

   if (*rst_nw_access) {
      outConf->flags = CMPTPcConfFlags::RstNwAccess;
   }

   if (*rst_pt_order) {
      outConf->flags = CMPTPcConfFlags::RstPtOrder;
   }

   return CMPTError::OK;
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExport(CMPTAcctGetPcConf);
   RegisterFunctionExport(CMPTGetDataSize);
}

} // namespace cafe::nn_cmpt
