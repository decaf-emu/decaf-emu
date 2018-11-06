#include "nn_ios_error.h"

namespace nn::ios
{

nn::Result
convertError(::ios::Error error)
{
   switch (error) {
   case ::ios::Error::OK:
      return ResultOK;
   case ::ios::Error::Access:
      return ResultAccess;
   case ::ios::Error::Exists:
      return ResultExists;
   case ::ios::Error::Intr:
      return ResultIntr;
   case ::ios::Error::Invalid:
      return ResultInvalid;
   case ::ios::Error::Max:
      return ResultMax;
   case ::ios::Error::NoExists:
      return ResultNoExists;
   case ::ios::Error::QEmpty:
      return ResultQEmpty;
   case ::ios::Error::QFull:
      return ResultQFull;
   case ::ios::Error::Unknown:
      return ResultUnknown;
   case ::ios::Error::NotReady:
      return ResultNotReady;
   case ::ios::Error::InvalidObjType:
      return ResultInvalidObjType;
   case ::ios::Error::InvalidVersion:
      return ResultInvalidVersion;
   case ::ios::Error::InvalidSigner:
      return ResultInvalidSigner;
   case ::ios::Error::FailCheckValue:
      return ResultFailCheckValue;
   case ::ios::Error::FailInternal:
      return ResultFailInternal;
   case ::ios::Error::FailAlloc:
      return ResultFailAlloc;
   case ::ios::Error::InvalidSize:
      return ResultInvalidSize;
   case ::ios::Error::NoLink:
      return ResultNoLink;
   case ::ios::Error::ANFailed:
      return ResultANFailed;
   case ::ios::Error::MaxSemCount:
      return ResultMaxSemCount;
   case ::ios::Error::SemUnavailable:
      return ResultSemUnavailable;
   case ::ios::Error::InvalidHandle:
      return ResultInvalidHandle;
   case ::ios::Error::InvalidArg:
      return ResultInvalidArg;
   case ::ios::Error::NoResource:
      return ResultNoResource;
   case ::ios::Error::Busy:
      return ResultBusy;
   case ::ios::Error::Timeout:
      return ResultTimeout;
   case ::ios::Error::Alignment:
      return ResultAlignment;
   case ::ios::Error::BSP:
      return ResultBSP;
   case ::ios::Error::DataPending:
      return ResultDataPending;
   case ::ios::Error::Expired:
      return ResultExpired;
   case ::ios::Error::NoReadAccess:
      return ResultNoReadAccess;
   case ::ios::Error::NoWriteAccess:
      return ResultNoWriteAccess;
   case ::ios::Error::NoReadWriteAccess:
      return ResultNoReadWriteAccess;
   case ::ios::Error::ClientTxnLimit:
      return ResultClientTxnLimit;
   case ::ios::Error::StaleHandle:
      return ResultStaleHandle;
   default:
      return ResultUnknownValue;
   }
}

} // namespace nn::ios
