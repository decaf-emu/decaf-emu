#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace cpu
{

namespace config
{

namespace jit
{

//! Enable usage of jit
extern bool enabled;

//! Use JIT in verification mode where it compares execution to interpreter
extern bool verify;

//! Select a single block (starting address) for verification (0 = verify everything)
extern uint32_t verify_addr;

//! JIT code cache size in megabytes
extern unsigned int code_cache_size_mb;

//! JIT data cache size in megabytes
extern unsigned int data_cache_size_mb;

//! List of JIT optimizations to enable
extern std::vector<std::string> opt_flags;

//! Treat .rodata sections as read-only regardless of RPL/RPX flags
extern bool rodata_read_only;

} // namespace jit

} // namespace config

} // namespace cpu
