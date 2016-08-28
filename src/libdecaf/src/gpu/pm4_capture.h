#pragma once
#include <cstdint>
#include <string>

namespace pm4
{

struct Buffer;

void
captureStart(const std::string &path);

void
capturePause();

void
captureResume();

void
captureStop();

bool
captureEnabled();

void
captureCommandBuffer(pm4::Buffer *buffer);

void
captureCpuFlush(void *buffer,
                uint32_t size);

void
captureGpuFlush(void *buffer,
                uint32_t size);

} // namespace pm4
