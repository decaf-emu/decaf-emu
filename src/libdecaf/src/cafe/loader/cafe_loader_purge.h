#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::loader
{

struct LOADED_RPL;

namespace internal
{

void
LiPurgeOneUnlinkedModule(virt_ptr<LOADED_RPL> rpl);

} // namespace internal

} // namespace cafe::loader
