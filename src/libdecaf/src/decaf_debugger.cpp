#include "debugger/debugger_ui.h"
#include "decaf.h"
#include <string>

namespace decaf
{

namespace debugger
{

void
initialise()
{
   auto configPath = makeConfigPath("imgui.ini");
   ::debugger::ui::initialise(configPath);
}

} // namespace debugger

} // namespace decaf