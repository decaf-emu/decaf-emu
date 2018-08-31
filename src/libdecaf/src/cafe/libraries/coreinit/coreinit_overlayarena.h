#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

BOOL
OSIsEnabledOverlayArena();

void
OSEnableOverlayArena(uint32_t unk,
                     virt_ptr<virt_addr> outAddr,
                     virt_ptr<uint32_t> outSize);

void
OSDisableOverlayArena();

void
OSGetOverlayArenaRange(virt_ptr<virt_addr> outAddr,
                       virt_ptr<uint32_t> outSize);

} // namespace cafe::coreinit
