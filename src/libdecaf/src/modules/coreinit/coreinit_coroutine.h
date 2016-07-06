#pragma once
#include "common/types.h"
#include "common/be_val.h"
#include "common/structsize.h"

namespace coreinit
{

struct OSCoroutine
{
   be_val<uint32_t> nia;
   be_val<uint32_t> cr;
   be_val<uint32_t> gqr1;
   be_val<uint32_t> gpr1;
   be_val<uint32_t> gpr2;
   be_val<uint32_t> gpr13_31[19];
   be_val<double> fpr14_31[18];
   be_val<float> ps14_31[18][2];
};
CHECK_OFFSET(OSCoroutine, 0x000, nia);
CHECK_OFFSET(OSCoroutine, 0x004, cr);
CHECK_OFFSET(OSCoroutine, 0x008, gqr1);
CHECK_OFFSET(OSCoroutine, 0x00C, gpr1);
CHECK_OFFSET(OSCoroutine, 0x010, gpr2);
CHECK_OFFSET(OSCoroutine, 0x014, gpr13_31);
CHECK_OFFSET(OSCoroutine, 0x060, fpr14_31);
CHECK_OFFSET(OSCoroutine, 0x0F0, ps14_31);
CHECK_SIZE(OSCoroutine, 0x180);

} // namespace coreinit
