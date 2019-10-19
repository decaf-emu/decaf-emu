#pragma once
#include <vulkan/vulkan.hpp>

#if defined(VK_USE_PLATFORM_XLIB_KHR)
// Are you fucking serious X headers???
#undef None
#undef Status
#undef True
#undef False
#undef Bool
#undef RootWindow
#undef CurrentTime
#undef Success
#undef DestroyAll
#undef COUNT
#undef CREATE
#undef DeviceAdded
#undef DeviceRemoved
#endif
