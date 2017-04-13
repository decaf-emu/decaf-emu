#include "debugger/debugger_ui_opengl.h"
#include "decaf_debugger.h"

namespace decaf
{

static DebugUiRenderer *
sUiRenderer = nullptr;

DebugUiRenderer *
createDebugGLRenderer()
{
   return new ::debugger::ui::RendererOpenGL { };
}

void
setDebugUiRenderer(DebugUiRenderer *renderer)
{
   sUiRenderer = renderer;
}

DebugUiRenderer *
getDebugUiRenderer()
{
   return sUiRenderer;
}

} // namespace decaf