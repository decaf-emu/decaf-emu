#ifndef DECAF_NOGL

#include "common/platform_opengl.h"

#ifdef DECAF_SDL
#include <SDL.h>
#endif

namespace platform
{

static void *
nullLookup(const char *name)
{
   return nullptr;
}

static void *
(*lookupFunction)(const char *name) = nullLookup;

void
setGLLookupFunction(void *(*func)(const char *name))
{
   lookupFunction = func;
}

void *
lookupGLFunction(const char *name)
{
   return (*lookupFunction)(name);
}

} // namespace platform

#endif
