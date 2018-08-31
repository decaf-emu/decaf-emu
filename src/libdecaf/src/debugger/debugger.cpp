#include "debugger.h"
#include "debugger_controller.h"
#include "debugger_server_gdb.h"
#include "debugger_ui.h"
#include "debugger_ui_manager.h"
#include "decaf.h"

#include <imgui.h>

namespace debugger
{

static Controller
sController;

static ui::Manager
sUiManager { &sController };

static GdbServer
sGdbServer { &sController, &sUiManager };

void
initialise(const std::string &config,
           ClipboardTextGetCallback getClipboardFn,
           ClipboardTextSetCallback setClipboardFn)
{
   ImGui::CreateContext();
   sUiManager.load(config, getClipboardFn, setClipboardFn);

   if (decaf::config::debugger::enabled
    && decaf::config::debugger::gdb_stub) {
      sGdbServer.start(decaf::config::debugger::gdb_stub_port);
   }
}

void
shutdown()
{
   ImGui::DestroyContext();
   // Force resume any paused cores.
   sController.resume();
}

void
handleDebugBreakInterrupt()
{
   sController.onDebugBreakInterrupt();
}

void
notifyEntry(uint32_t preinit,
            uint32_t entryPoint)
{
   if (decaf::config::debugger::enabled
    && decaf::config::debugger::break_on_entry) {
      if (preinit) {
         sController.addBreakpoint(preinit);
      }

      sController.addBreakpoint(entryPoint);
   }
}

void
draw(unsigned width, unsigned height)
{
   sGdbServer.process();
   sUiManager.draw(width, height);
}

namespace ui
{

bool
onMouseAction(MouseButton button,
              MouseAction action)
{
   return sUiManager.onMouseAction(button, action);
}

bool
onMouseMove(float x,
            float y)
{
   return sUiManager.onMouseMove(x, y);
}

bool
onMouseScroll(float x,
              float y)
{
   return sUiManager.onMouseScroll(x, y);
}

bool
onKeyAction(KeyboardKey key,
            KeyboardAction action)
{
   return sUiManager.onKeyAction(key, action);
}

bool
onText(const char *text)
{
   return sUiManager.onText(text);
}

} // namespace ui

} // namespace debugger
