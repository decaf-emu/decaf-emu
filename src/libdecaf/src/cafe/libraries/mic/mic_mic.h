#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::mic
{

using MICHandle = uint32_t;
using MICError = int32_t;

struct MICStatus;

MICHandle
MICInit(uint32_t a1,
        virt_ptr<void> a2,
        virt_ptr<void> a3,
        virt_ptr<MICError> outError);

MICError
MICUninit(MICHandle handle);

MICError
MICOpen(MICHandle handle);

MICError
MICClose(MICHandle handle);

MICError
MICGetStatus(MICHandle handle,
             virt_ptr<MICStatus> status);

} // namespace cafe::mic
