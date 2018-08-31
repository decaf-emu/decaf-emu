#pragma once
#include "gx2_enum.h"
#include "gx2r_buffer.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2r_aperture GX2R Display List
 * \ingroup gx2
 * @{
 */

void
GX2RBeginDisplayListEx(virt_ptr<GX2RBuffer> displayList,
                       uint32_t unused,
                       GX2RResourceFlags flags);

uint32_t
GX2REndDisplayList(virt_ptr<GX2RBuffer> displayList);

void
GX2RCallDisplayList(virt_ptr<GX2RBuffer> displayList,
                    uint32_t size);

void
GX2RDirectCallDisplayList(virt_ptr<GX2RBuffer> displayList,
                          uint32_t size);

/** @} */

} // namespace cafe::gx2
