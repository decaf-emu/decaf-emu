#pragma once
#include <string>
#include <vector>
#include <spdlog\spdlog.h>
#include <glbinding/gl/gl.h>
#include "decaf_input.h"

namespace decaf
{

void setSystemPath(const std::string &path);
void setGamePath(const std::string &path);
void setJitMode(bool enabled, bool debug = false);
void setKernelTraceEnabled(bool enabled);

void initLogging(std::vector<spdlog::sink_ptr> &sinks, spdlog::level::level_enum level);
bool initialise();
void start();
void shutdown();

void runGpuDriver();
void shutdownGpuDriver();
void getSwapBuffers(gl::GLuint* tv, gl::GLuint* drc);
float getAverageFps();

void setVpadCoreButtonCallback(input::ButtonStatus(*fn)(input::vpad::Channel channel, input::vpad::Core button));

} // namespace decaf
