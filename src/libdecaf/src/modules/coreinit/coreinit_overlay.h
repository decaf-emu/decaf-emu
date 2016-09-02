#pragma once
#include <common/types.h>
#include <common/be_val.h>

namespace coreinit
{

BOOL
OSIsEnabledOverlayArena();

void
OSEnableOverlayArena(uint32_t unk,
                     be_val<uint32_t> *addr,
                     be_val<uint32_t> *size);

void
OSDisableOverlayArena();

void
OSGetOverlayArenaRange(be_val<uint32_t> *addr,
                       be_val<uint32_t> *size);

} // namespace coreinit
