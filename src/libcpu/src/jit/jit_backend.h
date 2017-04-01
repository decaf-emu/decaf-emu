#pragma once
#include "jit_stats.h"
#include "state.h"
#include <cstdint>

namespace cpu
{

namespace jit
{

class JitBackend
{
public:
   virtual ~JitBackend() = default;

   //! Initialise core specific state.
   virtual Core *
   initialiseCore(uint32_t id) = 0;

   //! Clear any cached code for specified memory range.
   virtual void
   clearCache(uint32_t address, uint32_t size) = 0;

   //! Resume execution on current core.
   virtual void
   resumeExecution() = 0;

   //! Mark a region of memory as read only.
   virtual void
   addReadOnlyRange(uint32_t address, uint32_t size) = 0;

   //! Sample JIT stats.
   virtual bool
   sampleStats(JitStats &stats) = 0;

   //! Reset JIT profiling stats.
   virtual void
   resetProfileStats() = 0;

   //! Set which cores to profile
   virtual void
   setProfilingMask(unsigned mask) = 0;

   //! Get which cores are being profiled
   virtual unsigned
   getProfilingMask() = 0;

private:
};

} // namespace jit

} // namespace cpu
