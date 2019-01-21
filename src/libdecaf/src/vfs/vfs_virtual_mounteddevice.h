#pragma once
#include "vfs_virtual_node.h"

#include <memory>

namespace vfs
{

class Device;

struct VirtualMountedDevice : public VirtualNode
{
   VirtualMountedDevice(std::shared_ptr<Device> device) :
      VirtualNode(VirtualNode::MountedDevice),
      device(std::move(device))
   {
   }

   std::shared_ptr<Device> device;
};

} // namespace vfs
