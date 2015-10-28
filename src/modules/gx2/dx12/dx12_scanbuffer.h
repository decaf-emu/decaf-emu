#pragma once
#include "dx12_state.h"

struct DXScanBufferData {
   void alloc(int width, int height, DXGI_FORMAT fmt) {
      // Describe and create a Texture2D.
      D3D12_RESOURCE_DESC textureDesc = {};
      textureDesc.MipLevels = 1;
      textureDesc.Format = fmt;
      textureDesc.Width = width;
      textureDesc.Height = height;
      textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
      textureDesc.DepthOrArraySize = 1;
      textureDesc.SampleDesc.Count = 1;
      textureDesc.SampleDesc.Quality = 0;
      textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

      ThrowIfFailed(gDX.device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
         D3D12_HEAP_FLAG_NONE,
         &textureDesc,
         D3D12_RESOURCE_STATE_COPY_DEST,
         nullptr,
         IID_PPV_ARGS(&buffer)));

      // Describe and create a SRV for the texture.
      mView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      mView.Format = textureDesc.Format;
      mView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      mView.Texture2D.MipLevels = 1;

   }

   void release() {

   }

   operator D3D12_SHADER_RESOURCE_VIEW_DESC*() {
      return &mView;
   }

   operator ID3D12Resource*() {
      return buffer.Get();
   }

   D3D12_SHADER_RESOURCE_VIEW_DESC mView;
   ComPtr<ID3D12Resource> buffer;
   DXHeapItemPtr srv;

};