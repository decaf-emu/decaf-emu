#include "vfs_overlay_directoryiterator.h"

namespace vfs
{

OverlayDirectoryIterator::OverlayDirectoryIterator(User user,
                                                   Path path,
                                                   OverlayDevice *overlayDevice,
                                                   Error &error) :
   mUser(std::move(user)),
   mPath(std::move(path)),
   mDevice(overlayDevice)
{
   error = rewind();
}

Result<Status>
OverlayDirectoryIterator::readEntry()
{
   while (true) {
      auto result = mIterator.readEntry();
      if (result) {
         return result;
      }

      if (result.error() != Error::EndOfDirectory) {
         return result;
      }

      if (mDeviceIterator == mDevice->end()) {
         return Error::EndOfDirectory;
      }

      if (auto openResult = mDeviceIterator->second->openDirectory(mUser, mPath); !openResult) {
         return openResult.error();
      } else {
         mDeviceIterator++;
         mIterator = std::move(*openResult);
      }
   }
}

Error
OverlayDirectoryIterator::rewind()
{
   mDeviceIterator = mDevice->begin();
   auto result = mDeviceIterator->second->openDirectory(mUser, mPath);
   if (!result) {
      return result.error();
   }

   mIterator = std::move(*result);
   return Error::Success;
}

} // namespace vfs
