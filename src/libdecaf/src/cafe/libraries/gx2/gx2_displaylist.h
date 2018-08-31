#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_displaylist Display List
 * \ingroup gx2
 * @{
 */

constexpr auto GX2DisplayListAlign = 0x20u;

void
GX2BeginDisplayList(virt_ptr<void> displayList,
                    uint32_t bytes);

void
GX2BeginDisplayListEx(virt_ptr<void> displayList,
                      uint32_t bytes,
                      BOOL unk1);

uint32_t
GX2EndDisplayList(virt_ptr<void> displayList);

void
GX2DirectCallDisplayList(virt_ptr<void> displayList,
                         uint32_t bytes);

void
GX2CallDisplayList(virt_ptr<void> displayList,
                   uint32_t bytes);

BOOL
GX2GetDisplayListWriteStatus();

BOOL
GX2GetCurrentDisplayList(virt_ptr<virt_ptr<void>> outDisplayList,
                         virt_ptr<uint32_t> outSize);

void
GX2CopyDisplayList(virt_ptr<void> displayList,
                   uint32_t bytes);

void
GX2PatchDisplayList(virt_ptr<void> displayList,
                    GX2PatchShaderType type,
                    uint32_t byteOffset,
                    virt_ptr<void> shader);

/** @} */

} // namespace cafe::gx2
