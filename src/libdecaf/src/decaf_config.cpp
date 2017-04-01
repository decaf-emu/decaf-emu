#include "decaf_config.h"
#include "modules/coreinit/coreinit_enum.h"

namespace decaf
{

namespace config
{

namespace debugger
{

bool enabled = true;
bool break_on_entry = false;

} // namespace debugger

namespace gpu
{

bool debug = false;
std::vector<unsigned> debug_filters = {};

} // namespace gpu

namespace gx2
{

bool dump_textures = false;
bool dump_shaders = false;

} // namespace gx2

namespace jit
{

bool enabled = true;
bool verify = false;
uint32_t verify_addr = 0;
unsigned int cache_size_mb = 1024;
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

namespace log
{

bool async = false;
bool kernel_trace = false;
bool kernel_trace_res = false;
bool branch_trace = false;

std::vector<std::string> kernel_trace_filters =
{
   "+*",
   "-coreinit::__ghsLock",
   "-coreinit::__ghsUnlock",
   "-coreinit::__gh_errno_ptr",
   "-coreinit::__gh_set_errno",
   "-coreinit::__gh_get_errno",
   "-coreinit::__get_eh_globals",
   "-coreinit::OSGetTime",
   "-coreinit::OSGetSystemTime",
};

} // namespace log

namespace sound
{

bool dump_sounds = false;

} // namespace sound

namespace system
{

int region = static_cast<int>(coreinit::SCIRegion::USA);
std::string mlc_path = "mlc";
std::string sdcard_path = "sdcard";
std::string content_path = {};
double time_scale = 1.0;

} // namespace system

namespace ui
{

BackgroundColour background_colour = {153, 51, 51};

} // namespace ui

} // namespace config

} // namespace decaf
