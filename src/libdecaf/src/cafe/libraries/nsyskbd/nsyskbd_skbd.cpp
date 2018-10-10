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
SKBDGetChannelStatus(SKBDChannel channel,
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
SKBDGetKey(SKBDChannel channel,
           virt_ptr<SKBDKeyData> keyData)
{
   decaf_warn_stub();
   std::memset(keyData.get(), 0, sizeof(SKBDKeyData));
   keyData->channel = channel;
   return SKBDError::OK;
}

SKBDError
SKBDGetModState(SKBDChannel channel,
                virt_ptr<SKBDModState> outModState)
{
   decaf_warn_stub();
   *outModState = SKBDModState::NoModifiers;
   return SKBDError::OK;
}

SKBDError
SKBDResetChannel(SKBDChannel channel)
{
   decaf_warn_stub();
   return SKBDError::OK;
}

SKBDError
SKBDSetCountry(SKBDChannel channel,
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
