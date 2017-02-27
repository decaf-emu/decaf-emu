#pragma once
#include <common/teenyheap.h>
#include <cstdint>

namespace kernel
{

void
initialiseCodeHeap(uint32_t size);

TeenyHeap *
getCodeHeap();

void
getMEM1Bound(uint32_t *addr,
             uint32_t *size);

void
getMEM2Bound(uint32_t *addr,
             uint32_t *size);

} // namespace kernel