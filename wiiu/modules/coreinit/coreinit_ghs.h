#pragma once
#include "systemtypes.h"

BOOL
ghsLock();

BOOL
ghsUnlock();

void
ghs_set_errno(uint32_t err);

uint32_t
ghs_get_errno();

uint32_t
ghs_flock_ptr();

void
ghs_flock_file(uint32_t lockIx);
