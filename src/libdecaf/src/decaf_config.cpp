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
bool show_seg_view = true;
bool show_thread_view = true;
bool show_mem_view = true;
bool show_disasm_view = true;
bool show_reg_view = true;
bool show_stack_view = true;
bool show_stats_view = true;

} // namespace debugger

namespace gx2
{

bool dump_textures = false;
bool dump_shaders = false;

} // namespace gx2

namespace jit
{

bool enabled = true;
bool debug = false;

} // namespace jit

namespace log
{

bool kernel_trace = false;
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

namespace system
{

int region = static_cast<int>(coreinit::SCIRegion::US);
std::string system_path = "/undefined_system_path";
double time_scale = 1.0;

} // namespace system

} // namespace config

} // namespace decaf
