#pragma once
#include "tcl_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::tcl
{

using TCLApertureHandle = uint32_t;

TCLStatus
TCLAllocTilingAperture(phys_addr addr,
                       uint32_t pitch,
                       uint32_t height,
                       uint32_t bytesPerPixel,
                       uint32_t tileMode,
                       uint32_t endian,
                       virt_ptr<TCLApertureHandle> outHandle,
                       virt_ptr<virt_addr> outAddress);

TCLStatus
TCLFreeTilingAperture(TCLApertureHandle handle);

namespace internal
{

void
initialiseApertures();

} // namespace internal

} // namespace cafe::tcl
