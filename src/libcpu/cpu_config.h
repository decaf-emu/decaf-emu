#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace cpu
{

struct JitSettings
{
   //! Enable usage of jit
   bool enabled = true;

   //! Use JIT in verification mode where it compares execution to interpreter
   bool verify = false;

   //! Select a single block (starting address) for verification (0 = verify everything)
   uint32_t verifyAddress = 0u;

   //! JIT code cache size in megabytes
   unsigned int codeCacheSizeMB = 1024;

   //! JIT data cache size in megabytes
   unsigned int dataCacheSizeMB = 512;

   //! List of JIT optimizations to enable
   std::vector<std::string> optimisationFlags =
   {
      "BASIC",
      "DECONDITION",
      "DSE",
      "FOLD_CONSTANTS",
      "PPC_PAIRED_LWARX_STWCX",
      "X86_BRANCH_ALIGNMENT",
      "X86_CONDITION_CODES",
      "X86_FIXED_REGS",
      "X86_FORWARD_CONDITIONS",
      "X86_STORE_IMMEDIATE",
   };

   //! Treat .rodata sections as read-only regardless of RPL/RPX flags
   bool rodataReadOnly = true;
};

struct MemorySettings
{
   //! Whether page guards for write tracking is enabled or not.
   bool writeTrackEnabled = false;
};

struct Settings
{
   JitSettings jit;
   MemorySettings memory;
};

std::shared_ptr<const Settings> config();
void setConfig(const Settings &settings);

} // namespace cpu
