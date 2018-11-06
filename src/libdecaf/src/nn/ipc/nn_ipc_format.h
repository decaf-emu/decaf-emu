#pragma once
#include <libcpu/be2_struct.h>

namespace nn::ipc
{

/*
 * IOCTLV vec order:
 *  Vectors which are both input and output are treated as output vectors.
 *
 *  numVecIn = 1 + 2 * number of user output vectors
 *  numVecOut = 1 + 2 * number of user input vectors
 *
 *  vec[0] = response buffer
 *    ... user output vecs ...
 *  vec[numVecIn] = request buffer
 *    ... user input vecs ...
 */

struct RequestHeader
{
   be2_val<uint32_t> unk0x00;
   be2_val<uint32_t> service;
   be2_val<uint32_t> unk0x08;
   be2_val<uint32_t> command;
};

struct ResponseHeader
{
   be2_val<uint32_t> result;
};

struct ManagedBufferParameter
{
   be2_val<uint32_t> alignedBufferSize;
   be2_val<uint8_t> alignedBufferIndex;
   be2_val<uint8_t> unalignedBufferIndex;
   be2_val<uint8_t> unalignedBeforeBufferSize;
   be2_val<uint8_t> unalignedAfterBufferSize;
};

} // namespace nn::ipc
