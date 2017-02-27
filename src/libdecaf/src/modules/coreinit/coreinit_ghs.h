#pragma once
#include "ppcutils/wfunc_ptr.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/bitfield.h>
#include <common/structsize.h>

namespace coreinit
{

static const uint32_t
GHS_FOPEN_MAX = 0x14;

static const uint32_t
GHS_FLOCK_MAX = 0x64;

extern be_wfunc_ptr<void> *
p__atexit_cleanup;

extern be_wfunc_ptr<void> *
p__stdio_cleanup;

extern be_val<uint16_t> *
p__gh_FOPEN_MAX;

BITFIELD(_ghs_iobuf_bits, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, readwrite);
   BITFIELD_ENTRY(1, 1, bool, writable);
   BITFIELD_ENTRY(2, 1, bool, readable);
   BITFIELD_ENTRY(18, 14, uint32_t, channel);
BITFIELD_END

struct _ghs_iobuf
{
   be_ptr<uint8_t> next_ptr;
   be_ptr<uint8_t> base_ptr;
   be_val<int32_t> bytes_left;
   be_val<_ghs_iobuf_bits> info;
};
CHECK_OFFSET(_ghs_iobuf, 0x0, next_ptr);
CHECK_OFFSET(_ghs_iobuf, 0x4, base_ptr);
CHECK_OFFSET(_ghs_iobuf, 0x8, bytes_left);
CHECK_OFFSET(_ghs_iobuf, 0xc, info);
CHECK_SIZE(_ghs_iobuf, 0x10);

namespace internal
{

void
ghsInitExceptions();

void
ghsCleanupExceptions();

} // namespace internal

} // namespace coreinit
