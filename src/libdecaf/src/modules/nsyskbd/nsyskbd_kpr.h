#pragma once
#include "nsyskbd_enum.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/structsize.h>

namespace nsyskbd
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
   be_val<kpr_char_t> buffer[6];
   be_val<KPRMode> mode;
   uint8_t numCharsOut;
   uint8_t numCharsIn;
   PADDING(2);
   be_val<uint32_t> unk0x14;
};
CHECK_OFFSET(KPRQueue, 0x00, buffer);
CHECK_OFFSET(KPRQueue, 0x0C, mode);
CHECK_OFFSET(KPRQueue, 0x10, numCharsOut);
CHECK_OFFSET(KPRQueue, 0x11, numCharsIn);
CHECK_OFFSET(KPRQueue, 0x14, unk0x14);
CHECK_SIZE(KPRQueue, 0x18);

#pragma pack(pop)

void
KPRInitQueue(KPRQueue *queue);

void
KPRSetMode(KPRQueue *queue,
           KPRMode mode);

KPRMode
KPRGetMode(KPRQueue *queue);

void
KPRClearQueue(KPRQueue *queue);

uint8_t
KPRPutChar(KPRQueue *queue,
           kpr_char_t chr);

kpr_char_t
KPRGetChar(KPRQueue *queue);

kpr_char_t
KPRRemoveChar(KPRQueue *queue);

uint8_t
KPRLookAhead(KPRQueue *queue,
             be_val<kpr_char_t> *buffer,
             uint32_t size);

/** @} */

} // namespace nsyskbd
