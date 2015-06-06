#pragma once

template<typename Type>
static inline bool
testFlag(Type flags, Type flag)
{
   return !!(static_cast<unsigned>(flags) & static_cast<unsigned>(flag));
}

static inline uint32_t
alignUp(uint32_t value, uint32_t alignment)
{
   return (value + (alignment - 1)) & ~(alignment - 1);
}

static inline uint32_t
alignDown(uint32_t value, uint32_t alignment)
{
   return value & ~(alignment - 1);
}
