#include "nsyskbd.h"
#include "nsyskbd_skbd.h"

namespace nsyskbd
{

SKBDError
SKBDSetup(uint32_t unk_r3)
{
   return SKBDError::OK;
}

SKBDError
SKBDTeardown()
{
   return SKBDError::OK;
}

SKBDError
SKBDGetChannelStatus(uint32_t channel,
                     be_val<SKBDChannelStatus> *status)
{
   if (channel == 0) {
      *status = SKBDChannelStatus::Connected;
   } else {
      *status = SKBDChannelStatus::Disconnected;
   }

   return SKBDError::OK;
}

SKBDError
SKBDGetKey(uint32_t channel,
           SKBDKeyData *keyData)
{
   std::memset(keyData, 0, sizeof(SKBDKeyData));
   keyData->channel = channel;
   // TODO: Reverse & fill out keyData based on keyboard state.
   return SKBDError::OK;
}

SKBDError
SKBDGetModState(uint32_t channel,
                SKBDModState *modState)
{
   *modState = SKBDModState::NoModifiers;
   // TODO: Reverse & fill out keyData based on keyboard modifier state.
   return SKBDError::OK;
}

SKBDError
SKBDResetChannel(uint32_t channel)
{
   return SKBDError::OK;
}

SKBDError
SKBDSetCountry(uint32_t channel,
               SKBDCountry country)
{
   if (country >= SKBDCountry::Max) {
      return SKBDError::InvalidCountry;
   }

   return SKBDError::OK;
}

SKBDError
SKBDSetMode(uint32_t mode)
{
   return SKBDError::OK;
}

void
Module::registerSkbdFunctions()
{
   RegisterKernelFunction(SKBDSetup);
   RegisterKernelFunction(SKBDTeardown);
   RegisterKernelFunction(SKBDGetChannelStatus);
   RegisterKernelFunction(SKBDGetKey);
   RegisterKernelFunction(SKBDGetModState);
   RegisterKernelFunction(SKBDResetChannel);
   RegisterKernelFunction(SKBDSetCountry);
   RegisterKernelFunction(SKBDSetMode);
}

} // namespace nsyskbd
