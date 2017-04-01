#include "jit.h"
#include "jit_backend.h"

namespace cpu
{

namespace jit
{

static JitBackend *
sBackend = nullptr;

/**
 * Set the JIT backend to use.
 */
void
setBackend(JitBackend *backend)
{
   sBackend = backend;
}


/**
 * Return the current active JIT backend.
 */
JitBackend *
getBackend()
{
   return sBackend;
}


/**
 * Initialize JIT-related fields in a Core instance.
 */
Core *
initialiseCore(uint32_t id)
{
   if (sBackend) {
      return sBackend->initialiseCore(id);
   } else {
      return nullptr;
   }
}


/**
 * Clear the JIT cache for the given address range.
 *
 * This function must not be called while any JIT code is being executed.
 * There is no guarentee that only the selected address range will be cleared.
 */
void
clearCache(uint32_t address, uint32_t size)
{
   if (sBackend) {
      sBackend->clearCache(address, size);
   }
}


/**
 * Mark the given range of addresses as read-only for JIT optimization.
 */
void
addReadOnlyRange(uint32_t address, uint32_t size)
{
   if (sBackend) {
      sBackend->addReadOnlyRange(address, size);
   }
}


/**
 * Begin executing guest code on the current core.
 */
void
resume()
{
   sBackend->resumeExecution();
}

} // namespace jit

} // namespace cpu
