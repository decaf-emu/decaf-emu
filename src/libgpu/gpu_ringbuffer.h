#pragma once
#include <cstdint>
#include <gsl.h>

namespace gpu::ringbuffer
{

using Buffer = gsl::span<uint32_t>;

void
write(const Buffer &buffer);

Buffer
read();

bool
wait();

void
wake();

} // namespace gpu::ringbuffer
