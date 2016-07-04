#include "decaf_config.h"

namespace decaf
{

namespace config
{

namespace debugger
{

bool enabled = true;
bool break_on_entry = false;

} // namespace debugger

namespace gx2
{

bool dump_textures = false;
bool dump_shaders = false;

} // namespace gx2

namespace jit
{

bool enabled = false;
bool debug = false;

} // namespace jit

namespace log
{

bool kernel_trace = false;
bool branch_trace = false;
std::vector<std::string> kernel_trace_filters = {
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

namespace system
{

std::string system_path = "/undefined";

} // namespace system

} // namespace config

} // namespace decaf
