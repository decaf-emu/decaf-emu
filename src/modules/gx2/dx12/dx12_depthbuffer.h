#pragma once
#include "dx12_state.h"

struct DXDepthBufferData : public HostLookupItem<GX2DepthBuffer> {
   void alloc() {
      D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
      depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
      depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
      depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

      D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
      depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
      depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
      depthOptimizedClearValue.DepthStencil.Stencil = 0;

      ThrowIfFailed(gDX.device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, 
            source->surface.width, 
            source->surface.height, 
            1, 0, 1, 0, 
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
         D3D12_RESOURCE_STATE_DEPTH_WRITE,
         &depthOptimizedClearValue,
         IID_PPV_ARGS(&buffer)
         ));

      rtv = gDX.rtvHeap->alloc();
      gDX.device->CreateDepthStencilView(buffer.Get(), &depthStencilDesc, *rtv);
   }

   void release() {
   }

   ComPtr<ID3D12Resource> buffer;
   DXHeapItemPtr rtv;

};