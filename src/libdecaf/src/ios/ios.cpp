#include "ios.h"
#include "ios_alarm_thread.h"
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
   internal::startWorkerThread();
   kernel::start();
}

void
join()
{
   internal::joinWorkerThread();
   internal::joinAlarmThread();
   kernel::stop();
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
