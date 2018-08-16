#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

struct OSThread;

void
ghs_exit(int32_t code);

int32_t
gh_get_errno();

void
gh_set_errno(int32_t error);

namespace internal
{

void
initialiseGhs();

void
ghsExceptionInit(virt_ptr<OSThread> thread);

void
ghsExceptionCleanup(virt_ptr<OSThread> thread);

} // namespace internal

} // namespace cafe::coreinit
