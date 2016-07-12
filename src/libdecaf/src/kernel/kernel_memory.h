#pragma once
#include "common/types.h"
#include "common/teenyheap.h"

namespace kernel
{

void
initialiseCodeHeap(uint32_t size);

TeenyHeap *
getCodeHeap();

void
getMEM1Bound(uint32_t *addr, uint32_t *size);

void
getMEM2Bound(uint32_t *addr, uint32_t *size);

} // namespace kernel