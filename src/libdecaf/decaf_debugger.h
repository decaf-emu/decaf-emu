#pragma once

namespace decaf
{

class DebugUiRenderer
{
public:
   virtual void initialise() = 0;
   virtual void draw(unsigned width, unsigned height) = 0;
};

DebugUiRenderer *
createDebugGLRenderer();

void
setDebugUiRenderer(DebugUiRenderer *renderer);

DebugUiRenderer *
getDebugUiRenderer();

} // namespace decaf
