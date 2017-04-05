#pragma once
#include "nsyskbd_enum.h"

#include <cstdint>
#include <common/be_val.h>

namespace nsyskbd
{

/**
 * \defgroup nsyskbd_skbd SKBD
 * \ingroup nsyskbd
 * @{
 */

#pragma pack(push, 1)

struct SKBDKeyData
{
   be_val<uint8_t> channel;
   UNKNOWN(0x0F);
};
CHECK_OFFSET(SKBDKeyData, 0x00, channel);
CHECK_SIZE(SKBDKeyData, 0x10);

#pragma pack(pop)

SKBDError
SKBDSetup(uint32_t unk_r3);

SKBDError
SKBDTeardown();

SKBDError
SKBDGetChannelStatus(uint32_t channel,
                     be_val<SKBDChannelStatus> *status);

SKBDError
SKBDGetKey(uint32_t channel,
           SKBDKeyData *keyData);

SKBDError
SKBDGetModState(uint32_t channel,
                SKBDModState *modState);

SKBDError
SKBDResetChannel(uint32_t channel);

SKBDError
SKBDSetCountry(uint32_t channel,
               SKBDCountry country);

SKBDError
SKBDSetMode(uint32_t mode);

/** @} */

} // namespace nsyskbd
