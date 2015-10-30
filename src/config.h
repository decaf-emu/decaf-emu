#pragma once
#include <string>

namespace config
{

namespace dx12
{
extern bool use_warp;
}

namespace gx2
{
extern bool dump_textures;
extern bool dump_shaders;
}

namespace log
{
extern bool async;
extern bool to_file;
extern bool to_stdout;
extern bool kernel_trace;
extern std::string filename;
extern std::string level;
}

namespace jit
{
extern bool enabled;
extern bool debug;
}

namespace system
{
extern std::string system_path;
}

bool load(const std::string &path);

} // namespace config
