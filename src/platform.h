#pragma once

#include <ctime>
#include <thread>

namespace platform {

tm localtime(const std::time_t& time);
void set_thread_name(std::thread* thread, const std::string& threadName);

namespace ui {

void initialise();
void initialiseCore(int coreId);
void run();

int drcWidth();
int drcHeight();
int tvWidth();
int tvHeight();

}

}

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
#define PLATFORM_WINDOWS
#else
#define PLATFORM_POSIX
#endif
