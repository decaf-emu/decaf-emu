#include "coreinit.h"
#include "coreinit_driver.h"

namespace coreinit
{

static const char *
sDriverName = nullptr; // TODO: Game memory string

static bool
sDriverDone = false;

struct FSDriverInterface
{
   OSDriver_GetNameFn name;
   OSDriver_OnInitFn onInit;
   OSDriver_OnAcquiredForegroundFn onAcquiredForeground;
   OSDriver_OnReleasedForegroundFn onReleasedForeground;
   OSDriver_OnDoneFn onDone;
};

static FSDriverInterface
sDriverInterface;

namespace internal
{

static const char *
fsDriverName();

static void
fsDriverOnInit(uint32_t unk);

static void
fsDriverOnAcquiredForeground(uint32_t unk);

static void
fsDriverOnReleasedForeground(uint32_t unk);

static void
fsDriverOnDone(uint32_t unk);

const char *
fsDriverName()
{
   return sDriverName;
}

void
fsDriverOnInit(uint32_t unk)
{
}

void
fsDriverOnAcquiredForeground(uint32_t unk)
{
}

void
fsDriverOnReleasedForeground(uint32_t unk)
{
}

void
fsDriverOnDone(uint32_t unk)
{
   sDriverDone = true;
}

bool
fsDriverDone()
{
   return sDriverDone;
}

} // namespace internal

void
Module::registerFsDriverFunctions()
{
   RegisterInternalFunction(internal::fsDriverName, sDriverInterface.name);
   RegisterInternalFunction(internal::fsDriverOnInit, sDriverInterface.onInit);
   RegisterInternalFunction(internal::fsDriverOnAcquiredForeground, sDriverInterface.onAcquiredForeground);
   RegisterInternalFunction(internal::fsDriverOnReleasedForeground, sDriverInterface.onReleasedForeground);
   RegisterInternalFunction(internal::fsDriverOnDone, sDriverInterface.onDone);
}

} // namespace coreinit
