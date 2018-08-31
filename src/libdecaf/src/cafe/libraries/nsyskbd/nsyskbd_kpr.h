#pragma once
#include "nsyskbd_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::nsyskbd
{

/**
 * \defgroup nsyskbd_kbr KBR
 * \ingroup nsyskbd
 *
 * This is used for combining characters. Which is useful for ALT+Num unicode
 * characters and for typing japanese things where you input multiple characters
 * which combine together into one character.
 *
 * @{
 */

#pragma pack(push, 1)

using kpr_char_t = int16_t;

struct KPRQueue
{
   be2_array<kpr_char_t, 6> buffer;
   be2_val<KPRMode> mode;
   be2_val<uint8_t> numCharsOut;
   be2_val<uint8_t> numCharsIn;
   PADDING(2);
   be2_val<uint32_t> unk0x14;
};
CHECK_OFFSET(KPRQueue, 0x00, buffer);
CHECK_OFFSET(KPRQueue, 0x0C, mode);
CHECK_OFFSET(KPRQueue, 0x10, numCharsOut);
CHECK_OFFSET(KPRQueue, 0x11, numCharsIn);
CHECK_OFFSET(KPRQueue, 0x14, unk0x14);
CHECK_SIZE(KPRQueue, 0x18);

#pragma pack(pop)

void
KPRInitQueue(virt_ptr<KPRQueue> queue);

void
KPRSetMode(virt_ptr<KPRQueue> queue,
           KPRMode mode);

KPRMode
KPRGetMode(virt_ptr<KPRQueue> queue);

void
KPRClearQueue(virt_ptr<KPRQueue> queue);

uint8_t
KPRPutChar(virt_ptr<KPRQueue> queue,
           kpr_char_t chr);

kpr_char_t
KPRGetChar(virt_ptr<KPRQueue> queue);

kpr_char_t
KPRRemoveChar(virt_ptr<KPRQueue> queue);

uint8_t
KPRLookAhead(virt_ptr<KPRQueue> queue,
             virt_ptr<kpr_char_t> buffer,
             uint32_t size);

/** @} */

} // namespace cafe::nsyskbd
