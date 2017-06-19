#pragma once
#include <cstdint>

namespace nsysnet
{

int32_t
socket_lib_init();

int32_t
socket_lib_finish();

int32_t
socket(int32_t family,
       int32_t type,
       int32_t proto);

int32_t
socketclose(int32_t fd);

} // namespace nsysnet
