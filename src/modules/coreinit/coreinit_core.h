#pragma once
#include "systemtypes.h"

static const uint32_t CoreCount = 3;

uint32_t
OSGetCoreCount();

uint32_t
OSGetCoreId();

uint32_t
OSGetMainCoreId();

BOOL
OSIsMainCore();
