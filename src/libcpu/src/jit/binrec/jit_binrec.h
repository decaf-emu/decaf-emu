#pragma once
#include "state.h"
#include "espresso/espresso_instruction.h"
#include "jit/jit_codecache.h"
#include "jit/jit_backend.h"

#include <binrec++.h>
#include <vector>
#include <string>

namespace cpu
{

namespace jit
{

struct BinrecOptimisationFlags
{
   bool useChaining = false;
   unsigned int common = 0;
   unsigned int guest = 0;
   unsigned int host = 0;
};

class BinrecBackend;
struct VerifyBuffer;

struct BinrecCore : public Core
{
   BinrecBackend *backend;

   // JIT callback functions
   void *(*chainLookup)(BinrecCore *, ppcaddr_t);
   bool (*branchCallback)(BinrecCore *, ppcaddr_t);
   uint64_t (*mftbHandler)(BinrecCore *);
   BinrecCore *(*scHandler)(BinrecCore *, espresso::Instruction);
   BinrecCore *(*trapHandler)(BinrecCore *);

   // Lookup tables for fres/frsqrte instructions
   const uint16_t *fresTable;
   const uint16_t *frsqrteTable;

   // JIT verification buffer (local to jit::resume())
   VerifyBuffer *verifyBuffer;

   // HLE call flag (for JIT profiler)
   bool calledHLE;

   //! Trap Handler hit a breakpoint.
   bool hitBreakpoint;
};

using BinrecHandle = binrec::Handle<BinrecCore *>;
using BinrecEntry = BinrecCore * (*)(BinrecCore *core, uintptr_t membase);

class BinrecBackend : public JitBackend
{
public:
   BinrecBackend(size_t codeCacheSize,
                 size_t dataCacheSize);

   ~BinrecBackend() override;

   Core *
   initialiseCore(uint32_t id) override;

   void
   clearCache(uint32_t address,
              uint32_t size) override;

   void
   resumeExecution() override;

   void
   addReadOnlyRange(uint32_t address,
                    uint32_t size) override;

   bool
   sampleStats(JitStats &stats) override;

   void
   resetProfileStats() override;

   void
   setProfilingMask(unsigned mask) override;

   unsigned
   getProfilingMask() override;

public:
   void
   setOptFlags(const std::vector<std::string> &optList);

   void
   setVerifyEnabled(bool enabled, uint32_t address = 0);

   CodeBlock *
   getCodeBlock(BinrecCore *core, uint32_t address);

protected:
   BinrecHandle *createBinrecHandle();

   inline CodeBlock *
   getCodeBlockFast(BinrecCore *core, uint32_t address);

   CodeBlock *
   checkForCodeBlockTrampoline(uint32_t address);

   void resumeVerifyExecution();

   void
   verifyInit(Core *core, VerifyBuffer *verifyBuf);

   void
   verifyPre(Core *core,
             VerifyBuffer *verifyBuf,
             uint32_t cia,
             uint32_t instr);

   void
   verifyPost(Core *core,
              VerifyBuffer *verifyBuf,
              uint32_t cia,
              uint32_t instr);

   static void
   brVerifyPreHandler(BinrecCore *core, uint32_t address);

   static void
   brVerifyPostHandler(BinrecCore *core, uint32_t address);

private:
   CodeCache mCodeCache;
   std::array<BinrecHandle *, 3> mHandles;
   BinrecOptimisationFlags mOptFlags;
   std::vector<std::pair<ppcaddr_t, uint32_t>> mReadOnlyRanges;
   std::atomic<uint64_t> mTotalProfileTime { 0 };
   uint32_t mProfilingMask = 0;
   bool mVerifyEnabled = false;
   uint32_t mVerifyAddress = 0;
};

} // namespace jit

} // namespace cpu
