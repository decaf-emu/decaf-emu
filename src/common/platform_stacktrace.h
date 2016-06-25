#pragma once
#include <string>

namespace platform
{

struct StackTrace;

StackTrace * captureStackTrace();
void freeStackTrace(StackTrace *);
void printStackTrace(StackTrace *);

} // namespace platform
