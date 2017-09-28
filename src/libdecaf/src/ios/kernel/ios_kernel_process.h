#pragma once
#include "ios/ios_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <utility>

namespace ios::kernel
{

constexpr auto NumIosProcess = 14;

Error
IOS_GetCurrentProcessId();

Error
IOS_GetProcessName(ProcessId process,
                   char *buffer);

namespace internal
{

ProcessId
getCurrentProcessId();

phys_ptr<void>
allocProcessStatic(size_t size);

template<typename Type, typename... Args>
phys_ptr<Type>
allocProcessStatic(Args &&args...)
{
   auto ptr = phys_cast<Type>(allocProcessStatic(sizeof(Type)));
   // Construct Type at memory
   new (ptr.getRawPointer()) Type { std::forward(args)... };
   return ptr;
}

} // namespace internal

} // namespace ios::kernel
