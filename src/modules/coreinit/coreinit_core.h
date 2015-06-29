#pragma once
#include "systemtypes.h"

uint32_t
OSGetCoreCount();

uint32_t
OSGetCoreId();

uint32_t
OSGetMainCoreId();

BOOL
OSIsMainCore();
