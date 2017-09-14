#if 0
#include "ios_device.h"
#include "ios_ipc.h"
#include "fsa/fsa_device.h"
#include "im/im_device.h"
#include "mcp/mcp_device.h"
#include "socket/socket_device.h"
#include "usr_cfg/usr_cfg_device.h"

#include <map>
#include <string>
#include <fmt/format.h>

namespace ios
{

using DeviceCreatorFn = IOSDevice *(*)();


//! Map of device name to creation function.
static std::map<std::string, DeviceCreatorFn>
sDeviceMap;


//! Map of open IOSHandles to IOSDevice.
static std::map<IOSHandle, IOSDevice *>
sOpenDeviceMap;


static IOSError
iosOpen(const char *name,
        size_t nameLen,
        IOSOpenMode mode);

static IOSError
iosClose(IOSHandle handle);

static IOSDevice *
iosGetDevice(IOSHandle handle);


/**
 * Handles an incoming IPC request.
 */
void
iosDispatchIpcRequest(IPCBuffer *buffer)
{
   auto reply = IOSError::FailInternal;

   switch (buffer->command) {
   case IOSCommand::Open:
   {
      auto name = reinterpret_cast<const char *>(buffer->buffer1.getRawPointer());
      auto nameLen = buffer->args[1];
      auto mode = static_cast<IOSOpenMode>(buffer->args[2].value());
      reply = iosOpen(name, nameLen, mode);
      break;
   }
   case IOSCommand::Close:
   {
      reply = iosClose(buffer->handle);
      break;
   }
   case IOSCommand::Ioctl:
   {
      auto device = iosGetDevice(buffer->handle);

      if (!device) {
         reply = IOSError::InvalidHandle;
      } else {
         auto request = buffer->args[0];
         auto inBuf = buffer->buffer1.getRawPointer();
         auto inLen = buffer->args[2];
         auto outBuf = buffer->buffer2.getRawPointer();
         auto outLen = buffer->args[4];
         reply = device->ioctl(request, inBuf, inLen, outBuf, outLen);
      }
      break;
   }
   case IOSCommand::Ioctlv:
   {
      auto device = iosGetDevice(buffer->handle);

      if (!device) {
         reply = IOSError::InvalidHandle;
      } else {
         auto request = buffer->args[0];
         auto vecIn = buffer->args[1];
         auto vecOut = buffer->args[2];
         auto vec = reinterpret_cast<IOSVec *>(buffer->buffer1.getRawPointer());
         reply = device->ioctlv(request, vecIn, vecOut, vec);
      }
      break;
   }
   default:
      decaf_abort(fmt::format("Unimplemented IOSCommand {}", buffer->command));
   }

   // Setup reply
   buffer->prevHandle = buffer->handle;
   buffer->prevCommand = buffer->command;
   buffer->reply = reply;
   buffer->command = IOSCommand::Reply;
}


/**
 * Handle an IOS_Open request.
 *
 * \return
 * Returns an IOSHandle on success, or an IOSError code otherwise.
 */
IOSError
iosOpen(const char *name,
        size_t nameLen,
        IOSOpenMode mode)
{
   static IOSHandle DeviceHandles = 1;
   auto deviceName = std::string { name };

   // Lookup device by name
   auto deviceItr = sDeviceMap.find(deviceName);
   if (deviceItr == sDeviceMap.end()) {
      return IOSError::NoExists;
   }

   // Create device
   auto device = deviceItr->second();

   // Call device open method
   auto error = device->open(mode);

   if (error != IOSError::OK) {
      // Open failed, delete device
      delete device;
      return error;
   }

   // Open succeeded, register device to a unique handle
   auto handle = DeviceHandles++;
   device->setHandle(handle);
   device->setName(deviceName);
   sOpenDeviceMap[handle] = device;
   return static_cast<IOSError>(handle);
}


/**
 * Handle an IOS_Close request.
 *
 * \return
 * Returns an IOSError::OK on success, or an IOSError code otherwise.
 */
IOSError
iosClose(IOSHandle handle)
{
   auto device = iosGetDevice(handle);
   if (!device) {
      return IOSError::InvalidHandle;
   }

   auto reply = device->close();
   sOpenDeviceMap.erase(device->handle());
   delete device;
   return reply;
}


/**
 * Find an IOSDevice by it's handle.
 *
 * \return
 * Returns nullptr on failure, or a valid IOSDevice pointer on success.
 */
IOSDevice *
iosGetDevice(IOSHandle handle)
{
   auto deviceItr = sOpenDeviceMap.find(handle);
   if (deviceItr == sOpenDeviceMap.end()) {
      return nullptr;
   }

   return deviceItr->second;
}


template<typename DeviceType>
static IOSDevice *
createDevice()
{
   return new DeviceType {};
}

template<typename DeviceType>
static void
addDevice(const std::string &name)
{
   sDeviceMap[name] = &createDevice<DeviceType>;
}


/**
 * Initialise IOS devices.
 */
void
iosInitDevices()
{
   addDevice<ios::dev::fsa::FSADevice>("/dev/fsa");
   addDevice<ios::dev::im::IMDevice>("/dev/im");
   addDevice<ios::dev::mcp::MCPDevice>("/dev/mcp");
   addDevice<ios::dev::socket::SocketDevice>("/dev/socket");
   addDevice<ios::dev::usr_cfg::UserConfigDevice>("/dev/usr_cfg");
}

} // namespace ios
#endif