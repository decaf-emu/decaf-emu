#pragma once
#include <libdecaf/decaf.h>
#include <string>
#include <thread>
#include <spdlog/spdlog.h>

using namespace decaf::input;

extern std::shared_ptr<spdlog::logger>
gCliLog;

class DecafCLI
{
public:
   int run(const std::string &gamePath);

private:
};
