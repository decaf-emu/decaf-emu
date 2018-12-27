#pragma once
#include <functional>
#include <string>

#ifdef DECAF_VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace debugui
{

using ClipboardTextGetCallback = std::function<const char *()>;
using ClipboardTextSetCallback = std::function<void(const char*)>;

enum class KeyboardAction
{
   Press,
   Release
};

enum class MouseButton
{
   Left,
   Right,
   Middle,
   Unknown
};

enum class MouseAction
{
   Press,
   Release
};

enum class KeyboardKey
{
   Unknown,

   // This is only used for ImgUi in the debugger, so only the keys
   //  which are actually used are included here for now.
   Tab,
   LeftArrow,
   RightArrow,
   UpArrow,
   DownArrow,
   PageUp,
   PageDown,
   Home,
   End,
   Delete,
   Backspace,
   Enter,
   Escape,
   LeftControl,
   LeftShift,
   LeftAlt,
   LeftSuper,
   RightControl,
   RightShift,
   RightAlt,
   RightSuper,
   A, B, C, D, E, F,
   G, H, I, J, K, L,
   M, N, O, P, Q, R,
   S, T, U, V, W, X,
   Y, Z,
   F1, F2, F3, F4, F5, F6,
   F7, F8, F9, F10, F11, F12
};

class Renderer
{
public:
   virtual ~Renderer() = default;

   virtual bool onKeyAction(KeyboardKey key, KeyboardAction action) = 0;
   virtual bool onMouseAction(MouseButton button, MouseAction action) = 0;
   virtual bool onMouseMove(float x, float y) = 0;
   virtual bool onMouseScroll(float x, float y) = 0;
   virtual bool onText(const char *text) = 0;

   virtual void setClipboardCallbacks(ClipboardTextGetCallback getClipboardFn,
                                      ClipboardTextSetCallback setClipboardFn) = 0;
};

#ifdef DECAF_GL
class OpenGLRenderer : public Renderer
{
public:
   virtual ~OpenGLRenderer() override = default;
   virtual void draw(unsigned width, unsigned height) = 0;
};

OpenGLRenderer *
createOpenGLRenderer(const std::string &configPath);
#endif

#ifdef DECAF_VULKAN
struct VulkanRendererInfo
{
   vk::PhysicalDevice physDevice;
   vk::Device device;
   vk::Queue queue;
   vk::DescriptorPool descriptorPool;
   vk::RenderPass renderPass;
   vk::CommandPool commandPool;
};

class VulkanRenderer : public Renderer
{
public:
   virtual ~VulkanRenderer() override = default;
   virtual void draw(vk::CommandBuffer cmdBuffer, unsigned width, unsigned height) = 0;
};

VulkanRenderer *
createVulkanRenderer(const std::string &configPath, VulkanRendererInfo &info);
#endif

} // namespace debugui
