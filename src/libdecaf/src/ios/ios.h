#pragma once
#include <memory>

namespace vfs
{
class VirtualDevice;
}

namespace ios
{

void
start();

void
join();

void
setFileSystem(std::shared_ptr<vfs::VirtualDevice> fs);

std::shared_ptr<vfs::VirtualDevice>
getFileSystem();

} // namespace ios
