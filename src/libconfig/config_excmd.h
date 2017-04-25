#pragma once
#include <excmd.h>
#include <vector>

namespace config
{

std::vector<excmd::option_group *>
getExcmdGroups(excmd::parser &parser);

bool
loadFromExcmd(excmd::option_state &options);

} // namespace config
