#include "decaf_debug_api.h"

#include "cafe/libraries/gx2/gx2_internal_pm4cap.h"

namespace decaf::debug
{

Pm4CaptureState
pm4CaptureState()
{
   switch (cafe::gx2::internal::captureState()) {
   case cafe::gx2::internal::CaptureState::Disabled:
      return Pm4CaptureState::Disabled;
   case cafe::gx2::internal::CaptureState::Enabled:
      return Pm4CaptureState::Enabled;
   case cafe::gx2::internal::CaptureState::WaitEndNextFrame:
      return Pm4CaptureState::WaitEndNextFrame;
   case cafe::gx2::internal::CaptureState::WaitStartNextFrame:
      return Pm4CaptureState::WaitStartNextFrame;
   default:
      return Pm4CaptureState::Disabled;
   }
}

bool
pm4CaptureNextFrame()
{
   cafe::gx2::internal::captureNextFrame();
   return true;
}

bool
pm4CaptureBegin()
{
   cafe::gx2::internal::captureStartAtNextSwap();
   return true;
}

bool
pm4CaptureEnd()
{
   cafe::gx2::internal::captureStopAtNextSwap();
   return true;
}

} // namespace decaf::debug
