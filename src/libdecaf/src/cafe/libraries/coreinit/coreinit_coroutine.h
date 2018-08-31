#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_coroutine Coroutines
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSCoroutine
{
   be2_val<uint32_t> lr;
   be2_val<uint32_t> cr;
   be2_val<uint32_t> gqr1;
   be2_val<uint32_t> gpr1;
   be2_val<uint32_t> gpr2;
   be2_val<uint32_t> gpr13_31[19];
   be2_val<double> fpr14_31[18];
   be2_val<float> ps14_31[18][2];
};
CHECK_OFFSET(OSCoroutine, 0x000, lr);
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
OSInitCoroutine(virt_ptr<OSCoroutine> context,
                uint32_t entry,
                uint32_t stack);

uint32_t
OSLoadCoroutine(virt_ptr<OSCoroutine> coroutine,
                uint32_t returnValue);

uint32_t
OSSaveCoroutine(virt_ptr<OSCoroutine> coroutine);

void
OSSwitchCoroutine(virt_ptr<OSCoroutine> from,
                  virt_ptr<OSCoroutine> to);

/** @} */

} // namespace cafe::coreinit
