#pragma once
#include "dx12_state.h"
#include "dx12_utils.h"
#include "gpu/latte_untile.h"

struct DXTextureData : public HostLookupItem<GX2Texture> {
   void alloc() {
      textureWidth = dx12FixSize(source->surface.format, source->surface.width);
      textureHeight = dx12FixSize(source->surface.format, source->surface.height);

      // Describe and create a Texture2D.
      D3D12_RESOURCE_DESC textureDesc = {};
      textureDesc.MipLevels = 1;
      textureDesc.Format = dx12MakeFormat(source->surface.format);
      textureDesc.Width = textureWidth;
      textureDesc.Height = textureHeight;
      textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
      textureDesc.DepthOrArraySize = 1;
      textureDesc.SampleDesc.Count = 1;
      textureDesc.SampleDesc.Quality = 0;
      textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

      ThrowIfFailed(gDX.device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
         D3D12_HEAP_FLAG_NONE,
         &textureDesc,
         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
         nullptr,
         IID_PPV_ARGS(&buffer)));

      const UINT64 uploadBufferSize = GetRequiredIntermediateSize(buffer.Get(), 0, 1);

      ThrowIfFailed(gDX.device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
         D3D12_RESOURCE_STATE_GENERIC_READ,
         nullptr,
         IID_PPV_ARGS(&uploadBuffer)));

      // Describe and create a SRV for the texture.
      mView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      mView.Format = textureDesc.Format;
      mView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      mView.Texture2D.MipLevels = 1;
   }

   void release() {
   }

   void upload() {
      gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

      size_t rowPitch;
      latte::untileSurface(&source->surface, textureData, rowPitch);

      D3D12_SUBRESOURCE_DATA uploadData = {};
      uploadData.pData = &textureData[0];
      uploadData.RowPitch = rowPitch;
      uploadData.SlicePitch = rowPitch * textureHeight;

      UpdateSubresources(gDX.commandList.Get(), buffer.Get(), uploadBuffer.Get(), 0, 0, 1, &uploadData);

      gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
   }

   operator D3D12_SHADER_RESOURCE_VIEW_DESC*() {
      return &mView;
   }

   operator ID3D12Resource*() {
      return buffer.Get();
   }

   ComPtr<ID3D12Resource> buffer;
   DXHeapItemPtr srv;
   D3D12_SHADER_RESOURCE_VIEW_DESC mView;

   // TODO: Can probably use a shared big upload buffer...
   ComPtr<ID3D12Resource> uploadBuffer;

   // TODO: Make this a hash and use conversion instead...
   uint32_t textureWidth;
   uint32_t textureHeight;
   std::vector<UINT8> textureData;
};