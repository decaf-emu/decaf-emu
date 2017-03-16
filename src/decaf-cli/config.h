#pragma once
#include <string>

namespace config
{

namespace system
{

extern uint32_t timeout_ms;

} // namespace system

namespace log
{

extern bool to_file;
extern bool to_stdout;
extern std::string level;

} // namespace log

bool load(const std::string &path);
void save(const std::string &path);

} // namespace config
