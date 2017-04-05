#pragma once
#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_coroutine Coroutines
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

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

#pragma pack(pop)

void
OSInitCoroutine(OSCoroutine *context,
                uint32_t entry,
                uint32_t stack);

void
OSSwitchCoroutine(OSCoroutine *from,
                  OSCoroutine *to);

/** @} */

} // namespace coreinit
