#pragma once

#include <vulkan/vk_platform.h>

#ifdef VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

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
