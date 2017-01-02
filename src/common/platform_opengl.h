#pragma once

namespace platform
{

void
setGLLookupFunction(void *(*func)(const char *name));

void *
lookupGLFunction(const char *name);

} // namespace platform
