#pragma once
#include "vfs_directoryiterator.h"
#include "vfs_overlay_device.h"
#include "vfs_path.h"

namespace vfs
{

class OverlayDirectoryIterator : public DirectoryIteratorImpl
{
public:
   OverlayDirectoryIterator(User user,
                            Path path,
                            OverlayDevice *overlayDevice,
                            Error &error);
   ~OverlayDirectoryIterator() override = default;

   Result<Status> readEntry() override;
   Error rewind() override;

private:
   User mUser;
   Path mPath;
   OverlayDevice *mDevice;
   OverlayDevice::iterator mDeviceIterator;
   DirectoryIterator mIterator;
};

} // namespace vfs
