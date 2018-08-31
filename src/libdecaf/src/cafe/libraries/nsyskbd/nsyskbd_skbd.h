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

using SKBDChannel = uint8_t;

struct SKBDKeyData
{
   be2_val<SKBDChannel> channel;
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
SKBDGetChannelStatus(SKBDChannel channel,
                     virt_ptr<SKBDChannelStatus> outStatus);

SKBDError
SKBDGetKey(SKBDChannel channel,
           virt_ptr<SKBDKeyData> keyData);

SKBDError
SKBDGetModState(SKBDChannel channel,
                virt_ptr<SKBDModState> outModState);

SKBDError
SKBDResetChannel(SKBDChannel channel);

SKBDError
SKBDSetCountry(SKBDChannel channel,
               SKBDCountry country);

SKBDError
SKBDSetMode(uint32_t mode);

/** @} */

} // namespace cafe::nsyskbd
