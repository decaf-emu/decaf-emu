#pragma once
#include <string>

namespace platform
{

struct StackTrace;

StackTrace *
captureStackTrace();

void
freeStackTrace(StackTrace *trace);

void
printStackTrace(StackTrace *trace);

} // namespace platform
