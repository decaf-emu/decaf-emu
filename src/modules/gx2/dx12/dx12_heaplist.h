#pragma once
#include <memory>
#include <vector>
#include "dx12.h"
#include "dx12_heap.h"

class DXHeapList {
public:
   class Item {
      friend DXHeapList;

   public:
      Item() { }

      Item(DXHeapList *parent, int index)
         : mCpuHandle(parent->mHeap->GetCPUDescriptorHandleForHeapStart(), index, parent->mDescriptorSize),
         mGpuHandle(parent->mHeap->GetGPUDescriptorHandleForHeapStart(), index, parent->mDescriptorSize)
      {
      }

      ~Item()
      {
      }

      operator D3D12_CPU_DESCRIPTOR_HANDLE() {
         return mCpuHandle;
      }
      operator D3D12_CPU_DESCRIPTOR_HANDLE*() {
         return &mCpuHandle;
      }

      operator D3D12_GPU_DESCRIPTOR_HANDLE() {
         return mGpuHandle;
      }
      operator D3D12_GPU_DESCRIPTOR_HANDLE*() {
         return &mGpuHandle;
      }

   protected:
      CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuHandle;
      CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuHandle;

   };

   DXHeapList(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_DESC desc)
      : mSize(desc.NumDescriptors)
   {
      ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));
      mDescriptorSize = device->GetDescriptorHandleIncrementSize(desc.Type);
   }

   Item alloc() {
      if (mIndex >= mSize) {
         throw;
      }
      return Item(this, mIndex++);
   }

   void reset() {
      mIndex = 0;
   }

   operator ID3D12DescriptorHeap*() {
      return mHeap.Get();
   }

protected:
   ComPtr<ID3D12DescriptorHeap> mHeap;
   size_t mSize;
   int mDescriptorSize;
   uint32_t mIndex;

};
