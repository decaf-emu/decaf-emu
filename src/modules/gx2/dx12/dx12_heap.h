#pragma once
#include <memory>
#include <vector>
#include "dx12.h"

class DXHeap {
public:
   class Item {
      friend DXHeap;

   public:
      Item(DXHeap *parent, int index)
         : mParent(parent), mIndex(index),
         mCpuHandle(parent->mHeap->GetCPUDescriptorHandleForHeapStart(), index, parent->mDescriptorSize),
         mGpuHandle(parent->mHeap->GetGPUDescriptorHandleForHeapStart(), index, parent->mDescriptorSize)
      {
      }

      ~Item()
      {
         mParent->free(this);
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
      DXHeap *mParent;
      int mIndex;
      CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuHandle;
      CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuHandle;

   };

   typedef std::shared_ptr<Item> ItemPtr;

   DXHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_DESC desc)
      : mSize(desc.NumDescriptors)
   {
      mItems.resize(mSize, nullptr);
      ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));
      mDescriptorSize = device->GetDescriptorHandleIncrementSize(desc.Type);
   }

   operator ID3D12DescriptorHeap*() {
      return mHeap.Get();
   }

   ItemPtr alloc() {
      for (auto i = 0; i < mItems.size(); ++i) {
         if (mItems[i] == nullptr) {
            auto item = new Item(this, i);
            mItems[i] = item;
            return ItemPtr(item);
         }
      }
      assert(0);
      return ItemPtr();
   }

protected:
   void free(Item *item) {
      mItems[item->mIndex] = nullptr;
   }

   ComPtr<ID3D12DescriptorHeap> mHeap;
   size_t mSize;
   int mDescriptorSize;
   std::vector<Item*> mItems;

};

typedef DXHeap::ItemPtr DXHeapItemPtr;
