#pragma once
#include "dx12.h"

class DXDynBuffer {
public:
   class Allocation {
      friend class DXDynBuffer;

   public:
      Allocation()
      {
         mCpuAddr = nullptr;
         mView.BufferLocation = 0;
         mView.SizeInBytes = 0;
         mView.StrideInBytes = 0;
      }

      Allocation(uint8_t *cpuAddr, D3D12_GPU_VIRTUAL_ADDRESS gpuAddr, UINT stride, UINT size)
      {
         mCpuAddr = cpuAddr;
         mView.BufferLocation = gpuAddr;
         mView.StrideInBytes = stride;
         mView.SizeInBytes = size;
      }

      bool valid() const {
         return mView.BufferLocation != 0;
      }

      operator uint8_t*() {
         return mCpuAddr;
      }

      operator D3D12_GPU_VIRTUAL_ADDRESS() {
         return mView.BufferLocation;
      }

      operator D3D12_VERTEX_BUFFER_VIEW*() {
         return &mView;
      }

   protected:
      uint8_t *mCpuAddr;
      D3D12_VERTEX_BUFFER_VIEW mView;

   };

   DXDynBuffer(ID3D12Device *device, size_t size)
      : mSize(size), mOffset(0)
   {
      ThrowIfFailed(device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(size),
         D3D12_RESOURCE_STATE_GENERIC_READ,
         nullptr,
         IID_PPV_ARGS(&mBuffer)));

      CD3DX12_RANGE readRange(0, 0);
      ThrowIfFailed(mBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mCpuAddr)));
   }

   void reset() {
      mOffset = 0;
   }

   Allocation get(UINT stride, UINT size, void *data) {
      auto thisGpuAddr = mBuffer->GetGPUVirtualAddress() + mOffset;
      auto thisCpuAddr = mCpuAddr + mOffset;
      mOffset += size;
      Allocation alloc(thisCpuAddr, thisGpuAddr, stride, size);
      if (data) {
         memcpy(alloc.mCpuAddr, data, size);
      }
      return alloc;
   }

   ComPtr<ID3D12Resource> mBuffer;
   uint8_t *mCpuAddr;
   size_t mSize;
   size_t mOffset;

};
