#include "ios.h"
#include "ios_alarm_thread.h"
#include "ios_worker_thread.h"
#include "kernel/ios_kernel.h"

#include <memory>

namespace ios
{

static std::unique_ptr<::fs::FileSystem>
sFileSystem;

void
start()
{
   internal::startAlarmThread();
   internal::startWorkerThread();
   kernel::start();
}

void
join()
{
   kernel::join();
   internal::stopWorkerThread();
   internal::stopAlarmThread();
}

void
stop()
{
   kernel::stop();
   internal::stopWorkerThread();
   internal::stopAlarmThread();
}

void
setFileSystem(std::unique_ptr<::fs::FileSystem> fs)
{
   sFileSystem = std::move(fs);
}

::fs::FileSystem *
getFileSystem()
{
   return sFileSystem.get();
}

} // namespace ios
