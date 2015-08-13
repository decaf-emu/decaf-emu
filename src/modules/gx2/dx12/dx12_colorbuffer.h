#pragma once
#include "dx12_state.h"

struct DXColorBufferData : public HostLookupItem<GX2ColorBuffer> {
   void alloc() {
      // Describe and create a Texture2D.
      D3D12_RESOURCE_DESC textureDesc = {};
      textureDesc.MipLevels = 1;
      textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      textureDesc.Width = source->surface.width;
      textureDesc.Height = source->surface.height;
      textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
      textureDesc.DepthOrArraySize = 1;
      textureDesc.SampleDesc.Count = 1;
      textureDesc.SampleDesc.Quality = 0;
      textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

      D3D12_CLEAR_VALUE colorOptimizedClearValue = {};
      colorOptimizedClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      colorOptimizedClearValue.Color[0] = 0.0f;
      colorOptimizedClearValue.Color[1] = 0.0f;
      colorOptimizedClearValue.Color[2] = 0.0f;
      colorOptimizedClearValue.Color[3] = 1.0f;

      ThrowIfFailed(gDX.device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
         D3D12_HEAP_FLAG_NONE,
         &textureDesc,
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         &colorOptimizedClearValue,
         IID_PPV_ARGS(&buffer)));

      // Describe and create a SRV for the texture.
      D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
      srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      srvDesc.Format = textureDesc.Format;
      srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels = 1;

      srv = gDX.srvHeap->alloc();
      gDX.device->CreateShaderResourceView(buffer.Get(), &srvDesc, *srv);

      rtv = gDX.rtvHeap->alloc();
      gDX.device->CreateRenderTargetView(buffer.Get(), nullptr, *rtv);
   }

   void release() {
   }

   ComPtr<ID3D12Resource> buffer;
   DXHeapItemPtr srv;
   DXHeapItemPtr rtv;
};