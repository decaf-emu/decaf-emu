#include "decaf_config.h"

namespace decaf
{

namespace config
{

namespace debugger
{

bool enabled = true;
bool break_on_entry = false;
bool gdb_stub = false;
unsigned gdb_stub_port = 2159;

} // namespace debugger

namespace gx2
{

bool dump_textures = false;
bool dump_shaders = false;

} // namespace gx2

namespace log
{

bool async = false;
bool to_file = false;
bool to_stdout = false;
std::string level = "debug";
std::string directory = ".";
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

int region = 1;
std::string mlc_path = "mlc";
std::string slc_path = "slc";
std::string sdcard_path = "sdcard";
std::string content_path = {};
std::string resources_path = "resources";
double time_scale = 1.0;
std::vector<std::string> lle_modules;

} // namespace system

} // namespace config

} // namespace decaf
