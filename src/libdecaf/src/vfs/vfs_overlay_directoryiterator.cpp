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
   while (mDeviceIterator != mDevice->end()) {
      if (!mIterator.has_value()) {
         auto result = mDeviceIterator->second->openDirectory(mUser, mPath);
         if (!result) {
            mDeviceIterator++;
            continue;
         }

         mIterator = std::move(*result);
      }

      auto result = mIterator->readEntry();
      if (result) {
         return result;
      }

      if (result.error() == Error::EndOfDirectory) {
         mIterator.reset();
         mDeviceIterator++;
         continue;
      }
   }

   return Error::EndOfDirectory;
}

Error
OverlayDirectoryIterator::rewind()
{
   mDeviceIterator = mDevice->begin();
   mIterator.reset();

   // Find first device which can open directory
   while (mDeviceIterator != mDevice->end()) {
      auto result = mDeviceIterator->second->openDirectory(mUser, mPath);
      if (!result) {
         mDeviceIterator++;
         continue;
      }

      mIterator = std::move(*result);
      break;
   }

   return mIterator.has_value() ? Error::Success : Error::NotFound;
}

} // namespace vfs
