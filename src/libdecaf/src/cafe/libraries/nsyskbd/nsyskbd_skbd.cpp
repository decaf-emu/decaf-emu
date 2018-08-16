#include "nsyskbd.h"
#include "nsyskbd_skbd.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nsyskbd
{

SKBDError
SKBDSetup(uint32_t unk_r3)
{
   decaf_warn_stub();
   return SKBDError::OK;
}

SKBDError
SKBDTeardown()
{
   decaf_warn_stub();
   return SKBDError::OK;
}

SKBDError
SKBDGetChannelStatus(uint32_t channel,
                     virt_ptr<SKBDChannelStatus> outStatus)
{
   decaf_warn_stub();

   if (channel == 0) {
      *outStatus = SKBDChannelStatus::Connected;
   } else {
      *outStatus = SKBDChannelStatus::Disconnected;
   }

   return SKBDError::OK;
}

SKBDError
SKBDGetKey(uint32_t channel,
           virt_ptr<SKBDKeyData> keyData)
{
   decaf_warn_stub();
   std::memset(keyData.getRawPointer(), 0, sizeof(SKBDKeyData));
   keyData->channel = channel;
   return SKBDError::OK;
}

SKBDError
SKBDGetModState(uint32_t channel,
                virt_ptr<SKBDModState> outModState)
{
   decaf_warn_stub();
   *outModState = SKBDModState::NoModifiers;
   return SKBDError::OK;
}

SKBDError
SKBDResetChannel(uint32_t channel)
{
   decaf_warn_stub();
   return SKBDError::OK;
}

SKBDError
SKBDSetCountry(uint32_t channel,
               SKBDCountry country)
{
   decaf_warn_stub();
   if (country >= SKBDCountry::Max) {
      return SKBDError::InvalidCountry;
   }

   return SKBDError::OK;
}

SKBDError
SKBDSetMode(uint32_t mode)
{
   decaf_warn_stub();
   return SKBDError::OK;
}

void
Library::registerSkbdSymbols()
{
   RegisterFunctionExport(SKBDSetup);
   RegisterFunctionExport(SKBDTeardown);
   RegisterFunctionExport(SKBDGetChannelStatus);
   RegisterFunctionExport(SKBDGetKey);
   RegisterFunctionExport(SKBDGetModState);
   RegisterFunctionExport(SKBDResetChannel);
   RegisterFunctionExport(SKBDSetCountry);
   RegisterFunctionExport(SKBDSetMode);
}

} // namespace cafe::nsyskbd
