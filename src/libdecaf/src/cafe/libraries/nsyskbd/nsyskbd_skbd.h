#pragma once
#include "nsyskbd_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::nsyskbd
{

/**
 * \defgroup nsyskbd_skbd SKBD
 * \ingroup nsyskbd
 * @{
 */

#pragma pack(push, 1)

struct SKBDKeyData
{
   be2_val<uint8_t> channel;
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
                     virt_ptr<SKBDChannelStatus> outStatus);

SKBDError
SKBDGetKey(uint32_t channel,
           virt_ptr<SKBDKeyData> keyData);

SKBDError
SKBDGetModState(uint32_t channel,
                virt_ptr<SKBDModState> outModState);

SKBDError
SKBDResetChannel(uint32_t channel);

SKBDError
SKBDSetCountry(uint32_t channel,
               SKBDCountry country);

SKBDError
SKBDSetMode(uint32_t mode);

/** @} */

} // namespace cafe::nsyskbd
