#pragma once
#include "decaf_eventlistener.h"
#include "decaf_graphics.h"
#include "decaf_input.h"

#include <string>
#include <vector>
#include <memory>

namespace decaf
{

std::string
makeConfigPath(const std::string &filename);

bool
createConfigDirectory();

std::string
getResourcePath(const std::string &filename);

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
