#pragma once
#include "gx2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_draw Draw
 * \ingroup gx2
 * @{
 */

void
GX2SetAttribBuffer(uint32_t index,
                   uint32_t size,
                   uint32_t stride,
                   virt_ptr<void> buffer);

void
GX2DrawEx(GX2PrimitiveMode mode,
          uint32_t count,
          uint32_t offset,
          uint32_t numInstances);

void
GX2DrawEx2(GX2PrimitiveMode mode,
           uint32_t count,
           uint32_t offset,
           uint32_t numInstances,
           uint32_t baseInstance);

void
GX2DrawIndexedEx(GX2PrimitiveMode mode,
                 uint32_t count,
                 GX2IndexType indexType,
                 virt_ptr<void> indices,
                 uint32_t offset,
                 uint32_t numInstances);

void
GX2DrawIndexedEx2(GX2PrimitiveMode mode,
                  uint32_t count,
                  GX2IndexType indexType,
                  virt_ptr<void> indices,
                  uint32_t offset,
                  uint32_t numInstances,
                  uint32_t baseInstance);

void
GX2DrawIndexedImmediateEx(GX2PrimitiveMode mode,
                          uint32_t count,
                          GX2IndexType indexType,
                          virt_ptr<void> indices,
                          uint32_t offset,
                          uint32_t numInstances);

void
GX2SetPrimitiveRestartIndex(uint32_t index);

/** @} */

} // namespace cafe::gx2
