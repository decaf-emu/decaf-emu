#pragma once
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include "decaf_config.h"
#include "decaf_eventlistener.h"
#include "decaf_graphics.h"
#include "decaf_input.h"

namespace decaf
{

std::string
makeConfigPath(const std::string &filename);

bool
createConfigDirectory();

bool
initialise(const std::string &gamePath);

void
start();

bool
hasExited();

int
waitForExit();

void
shutdown();

} // namespace decaf
