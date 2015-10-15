#pragma once
#include "types.h"

using IOHandle = uint32_t;

static const IOHandle IOInvalidHandle = -1;

enum class IOError
{
   OK       = 0,
   Generic  = -1
};
