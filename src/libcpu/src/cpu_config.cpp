#include "cpu_config.h"

namespace cpu
{

namespace config
{

namespace jit
{

bool enabled = true;
bool verify = false;
uint32_t verify_addr = 0;
unsigned int code_cache_size_mb = 1024;
unsigned int data_cache_size_mb = 512;
bool rodata_read_only = true;

std::vector<std::string> opt_flags =
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

} // namespace jit

} // namespace config

} // namespace cpu
