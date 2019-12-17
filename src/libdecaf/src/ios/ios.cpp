#include "ios.h"
#include "ios_alarm_thread.h"
#include "ios_network_thread.h"
#include "ios_worker_thread.h"
#include "ios/kernel/ios_kernel.h"
#include "vfs/vfs_virtual_device.h"

#include <memory>

namespace ios
{

static std::shared_ptr<vfs::VirtualDevice> sFileSystem;

void
start()
{
   internal::startAlarmThread();
   internal::startNetworkTaskThread();
   internal::startWorkerThread();
   kernel::start();
}

void
join()
{
   kernel::join();
   internal::stopWorkerThread();
   internal::stopNetworkTaskThread();
   internal::stopAlarmThread();
}

void
stop()
{
   kernel::stop();
   internal::stopWorkerThread();
   internal::stopNetworkTaskThread();
   internal::stopAlarmThread();
}

void
setFileSystem(std::shared_ptr<vfs::VirtualDevice> root)
{
   sFileSystem = std::move(root);
}

std::shared_ptr<vfs::VirtualDevice>
getFileSystem()
{
   return sFileSystem;
}

} // namespace ios
