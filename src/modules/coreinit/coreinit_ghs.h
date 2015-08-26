#pragma once
#include "systemtypes.h"

#define GHS_FOPEN_MAX 0x14

extern be_wfunc_ptr<void>*
p__atexit_cleanup;

extern be_wfunc_ptr<void>*
p__stdio_cleanup;

extern be_wfunc_ptr<void, be_ptr<void>*>*
p___cpp_exception_init_ptr;

extern be_wfunc_ptr<void, be_ptr<void>*>*
p___cpp_exception_cleanup_ptr;

extern be_val<uint16_t>*
p__gh_FOPEN_MAX;

struct _ghs_iobuf {
   UNKNOWN(0x10);
};

typedef _ghs_iobuf __ghs_iob[GHS_FOPEN_MAX];
extern __ghs_iob* p_iob;

typedef be_ptr<void> __ghs_iob_lock[GHS_FOPEN_MAX + 1];
extern __ghs_iob_lock* p_iob_lock;

BOOL
ghsLock();

BOOL
ghsUnlock();

void
ghs_set_errno(uint32_t err);

uint32_t
ghs_get_errno();

be_ptr<void>*
ghs_flock_ptr(void *file);

void
ghs_flock_file(uint32_t lockIx);
