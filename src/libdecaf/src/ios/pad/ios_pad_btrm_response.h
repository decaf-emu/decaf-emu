#pragma once
#include <libcpu/be2_struct.h>

namespace ios::pad
{

/**
 * \ingroup ios_pad
 * @{
 */

#pragma pack(push, 1)

struct BtrmResponsePpcInitDone
{
   be2_val<uint32_t> unk0x00;
   be2_array<char, 6> unk0x04;
   be2_val<uint8_t> btChipId;
   be2_val<uint16_t> unk0x0B;
   be2_val<uint16_t> btChipBuildNumber;
   be2_val<uint8_t> unk0x0F;
};
CHECK_OFFSET(BtrmResponsePpcInitDone, 0x00, unk0x00);
CHECK_OFFSET(BtrmResponsePpcInitDone, 0x04, unk0x04);
CHECK_OFFSET(BtrmResponsePpcInitDone, 0x0A, btChipId);
CHECK_OFFSET(BtrmResponsePpcInitDone, 0x0B, unk0x0B);
CHECK_OFFSET(BtrmResponsePpcInitDone, 0x0D, btChipBuildNumber);
CHECK_OFFSET(BtrmResponsePpcInitDone, 0x0F, unk0x0F);

struct BtrmResponse
{
   union {
      be2_array<uint8_t, 0x1000> data;

      be2_struct<BtrmResponsePpcInitDone> ppcInitDone;
   };

   UNKNOWN(0xC);
};
CHECK_SIZE(BtrmResponse, 0x100C);

#pragma pack(pop)

/** @} */

} // namespace ios::pad
