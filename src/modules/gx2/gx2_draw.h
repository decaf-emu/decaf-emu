#pragma once
#include "types.h"
#include "modules/gx2/gx2_enum.h"

void
GX2SetAttribBuffer(uint32_t index,
                   uint32_t size,
                   uint32_t stride,
                   void *buffer);

void
GX2DrawEx(GX2PrimitiveMode::Value mode,
          uint32_t numVertices,
          uint32_t offset,
          uint32_t numInstances);

void
GX2DrawIndexedEx(GX2PrimitiveMode::Value mode,
                 uint32_t numVertices,
                 GX2IndexType::Value indexType,
                 void *indices,
                 uint32_t offset,
                 uint32_t numInstances);

void
GX2SetPrimitiveRestartIndex(uint32_t index);
