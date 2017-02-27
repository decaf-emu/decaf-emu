#pragma once
#include <common/be_val.h>
#include <cstdint>

namespace mic
{

using MICHandle = uint32_t;

MICHandle
MICInit(uint32_t type,
        void *,
        void *,
        be_val<int32_t> *result);

int
MICOpen(MICHandle handle);

int
MICGetStatus(MICHandle handle,
             void *statusOut);

} // namespace mic
