#pragma once
#include "ppcutils/va_list.h"
#include <cstdint>

namespace coreinit
{

namespace internal
{

bool
formatStringV(const char *fmt,
              ppctypes::va_list *list,
              std::string &output);

int
formatStringV(char *buffer,
              uint32_t len,
              const char *fmt,
              ppctypes::va_list *list);

} // namespace internal

} // namespace coreinit
