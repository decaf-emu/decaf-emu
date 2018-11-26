#ifndef H264_ENUM_H
#define H264_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_ENTER(cafe)
ENUM_NAMESPACE_ENTER(h264)

ENUM_BEG(EntryPoint, int8_t)
   ENUM_VALUE(InitParam,            4)
   ENUM_VALUE(SetParam,             5)
   ENUM_VALUE(Open,                 6)
   ENUM_VALUE(Begin,                7)
   ENUM_VALUE(SetBitStream,         8)
   ENUM_VALUE(Execute,              9)
   ENUM_VALUE(End,                  10)
   ENUM_VALUE(Close,                11)
ENUM_END(EntryPoint)

ENUM_BEG(H264Error, int32_t)
   ENUM_VALUE(OK,                   0)
   ENUM_VALUE(InvalidPps,           24)
   ENUM_VALUE(InvalidSps,           26)
   ENUM_VALUE(InvalidSliceHeader,   61)
   ENUM_VALUE(GenericError,         0x1000000)
   ENUM_VALUE(InvalidParameter,     0x1010000)
   ENUM_VALUE(OutOfMemory,          0x1020000)
   ENUM_VALUE(InvalidProfile,       0x1080000)
ENUM_END(H264Error)

ENUM_BEG(H264Parameter, int32_t)
   ENUM_VALUE(FramePointerOutput,   1)
   ENUM_VALUE(OutputPerFrame,       0x20000002)
   ENUM_VALUE(Unknown0x20000010,    0x20000010)
   ENUM_VALUE(Unknown0x20000030,    0x20000030)
   ENUM_VALUE(Unknown0x20000040,    0x20000040)
   ENUM_VALUE(UserMemory,           0x70000001)
ENUM_END(H264Parameter)

ENUM_BEG(SliceType, int32_t)
   ENUM_VALUE(P,                    0)
   ENUM_VALUE(B,                    1)
   ENUM_VALUE(I,                    2)

   ENUM_VALUE(EP,                   0)
   ENUM_VALUE(EB,                   1)
   ENUM_VALUE(EI,                   2)
   ENUM_VALUE(SP,                   3)
   ENUM_VALUE(SI,                   4)

   ENUM_VALUE(OnlyP,                5)
   ENUM_VALUE(OnlyB,                6)
   ENUM_VALUE(OnlyI,                7)

   ENUM_VALUE(OnlyEP,               5)
   ENUM_VALUE(OnlyEB,               6)
   ENUM_VALUE(OnlyEI,               7)
   ENUM_VALUE(OnlySP,               8)
   ENUM_VALUE(OnlySI,               9)
ENUM_END(SliceType)

ENUM_BEG(NaluType, int32_t)
   ENUM_VALUE(NonIdr,               1)
   ENUM_VALUE(DataPartitionA,       2)
   ENUM_VALUE(DataPartitionB,       3)
   ENUM_VALUE(DataPartitionC,       4)
   ENUM_VALUE(Idr,                  5)
   ENUM_VALUE(Sei,                  6)
   ENUM_VALUE(Sps,                  7)
   ENUM_VALUE(Pps,                  8)
   ENUM_VALUE(Aud,                  9)
   ENUM_VALUE(EndOfSequence,        10)
   ENUM_VALUE(EndOfStream,          11)
   ENUM_VALUE(Filler,               12)
   ENUM_VALUE(SpsExt,               13)
   ENUM_VALUE(PrefixNal,            14)
   ENUM_VALUE(SubsetSps,            15)
   ENUM_VALUE(Dps,                  16)
   ENUM_VALUE(CodedSliceAux,        19)
   ENUM_VALUE(CodedSliceSvcExt,     20)
ENUM_END(NaluType)

ENUM_NAMESPACE_EXIT(h264)
ENUM_NAMESPACE_EXIT(cafe)

#include <common/enum_end.h>

#endif // ifdef SWKBD_ENUM_H
