#pragma once
#include "debugui_window.h"

#include <libdecaf/decaf_debug_api.h>
#include <chrono>

namespace debugui
{

class ThreadsWindow : public Window
{
public:
   ThreadsWindow(const std::string &name);
   virtual ~ThreadsWindow() = default;

   virtual void
   draw();

private:
   std::vector<decaf::debug::CafeThread> mThreadsCache;
};

} // namespace debugui
