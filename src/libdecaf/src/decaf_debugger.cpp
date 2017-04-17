#include "debugger/debugger_ui_opengl.h"
#include "decaf_debugger.h"
#include <common/decaf_assert.h>

namespace decaf
{

static DebugUiRenderer *
sUiRenderer = nullptr;

DebugUiRenderer *
createDebugGLRenderer()
{
#ifndef DECAF_NOGL
   return new ::debugger::ui::RendererOpenGL { };
#else
   decaf_abort("libdecaf was built with OpenGL support disabled");
#endif // DECAF_NOGL
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