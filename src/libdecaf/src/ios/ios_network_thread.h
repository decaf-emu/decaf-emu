#pragma once
#include <ares.h>
#include <functional>
#include <uv.h>

namespace ios::internal
{

using NetworkTask = std::function<void()>;

void
startNetworkTaskThread();

void
stopNetworkTaskThread();

void
submitNetworkTask(NetworkTask task);

uv_loop_t *
networkUvLoop();

ares_channel
networkAresChannel();

} // namespace ios::internal
