#pragma once
#include <memory>
#include <vector>
#include "dx12.h"
#include "dx12_fetchshader.h"
#include "dx12_utils.h"
#include "modules/gx2/gx2_shaders.h"
#include "modules/gx2/gx2_debug.h"
#include "gpu/latte.h"
#include "gpu/hlsl/hlsl.h"
#include "utils/crc32.h"

class DXPipelineMgr {
public:
   class Item {
      friend DXPipelineMgr;

   public:
      Item(DXPipelineMgr *parent)
         : mParent(parent)
      {
      }

      ~Item()
      {
      }

   protected:
      DXPipelineMgr *mParent;
      GX2FetchShader *mFetchShader;
      GX2PixelShader *mPixelShader;
      GX2VertexShader *mVertexShader;
      ComPtr<ID3D12PipelineState> mPipelineState;

   };

   typedef std::shared_ptr<Item> ItemPtr;

   DXPipelineMgr()
   {
   }

   ComPtr<ID3D12PipelineState> findOrCreate()
   {
      for (auto &i : mItems) {
         if (i->mFetchShader == gDX.state.fetchShader &&
            i->mVertexShader == gDX.state.vertexShader &&
            i->mPixelShader == gDX.state.pixelShader) {
            return i->mPipelineState;
         }
      }

      return create();
   }

private:
   ComPtr<ID3D12PipelineState> create()
   {
      // Print Fetch Shader Data
      {
         auto shader = gDX.state.fetchShader;
         auto dataPtr = (FetchShaderInfo*)(void*)shader->data;
         gLog->debug("Fetch Shader [{}]:\n", (void*)shader);
         gLog->debug("  Type: {}\n", dataPtr->type);
         gLog->debug("  Tess: {}\n", dataPtr->tessMode);
         for (auto i = 0u; i < shader->attribCount; ++i) {
            auto attrib = dataPtr->attribs[i];
            gLog->debug("  Attrib[{}] L:{}, B:{}, O:{}, F:{}, T:{}, A:{}, M:{:08x}, E:{}\n", i,
               (int32_t)attrib.location,
               (int32_t)attrib.buffer,
               (int32_t)attrib.offset,
               (int32_t)attrib.format,
               (int32_t)attrib.type,
               (int32_t)attrib.aluDivisor,
               (uint32_t)attrib.mask,
               (int32_t)attrib.endianSwap);
         }
      }
      // Print Vertex Shader Data
      uint32_t vertexShaderCrc;
      {
         auto shader = gDX.state.vertexShader;
         vertexShaderCrc = crc32(shader->data, shader->size);
         gLog->debug("Pixel Shader [{}] ({:08x})\n", (void*)shader, vertexShaderCrc);
         GX2DumpShader(shader);
      }
      // Print Pixel Shader Data
      uint32_t pixelShaderCrc;
      {
         auto shader = gDX.state.pixelShader;
         pixelShaderCrc = crc32(shader->data, shader->size);
         gLog->debug("Pixel Shader [{}] ({:08x})\n", (void*)shader, pixelShaderCrc);
         GX2DumpShader(shader);
      }


      auto fetchShader = gDX.state.fetchShader;
      auto vertexShader = gDX.state.vertexShader;
      auto pixelShader = gDX.state.pixelShader;
      auto fetchData = (FetchShaderInfo*)(void*)fetchShader->data;

      std::string hlsl;
      {
         latte::Shader decVertexShader, decPixelShader;
         latte::decode(decVertexShader, { vertexShader->data.get(), vertexShader->size });
         latte::decode(decPixelShader, { pixelShader->data.get(), pixelShader->size });
         hlsl::generateHLSL({ (GX2AttribStream*)fetchData->attribs, fetchShader->attribCount }, decVertexShader, decPixelShader, hlsl);
         gLog->debug("Compiled Shader:\n{}\n", hlsl);
      }

      ComPtr<ID3DBlob> vertexShaderBlob;
      ComPtr<ID3DBlob> pixelShaderBlob;

#ifdef _DEBUG
      // Enable better shader debugging with the graphics debugging tools.
      UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
      UINT compileFlags = 0;
#endif

      ComPtr<ID3DBlob> errorBlob;
      HRESULT hr;
      hr = D3DCompile(hlsl.c_str(), hlsl.size(), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShaderBlob, &errorBlob);
      if (FAILED(hr)) {
         gLog->warn("Shader Source:\n{}\n", hlsl);
         gLog->warn("Vertex Shader Compilation Failed:\n{}", (char*)errorBlob->GetBufferPointer());
         throw;
      }
      hr = D3DCompile(hlsl.c_str(), hlsl.size(), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShaderBlob, &errorBlob);
      if (FAILED(hr)) {
         gLog->warn("Shader Source:\n{}\n", hlsl);
         gLog->warn("Pixel Shader Compilation Failed:\n{}", (char*)errorBlob->GetBufferPointer());
         throw;
      }

      DXGI_FORMAT inputElementFormat;
      std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
      for (auto i = 0u; i < fetchShader->attribCount; ++i) {
         auto &attrib = fetchData->attribs[i];

         switch (attrib.format) {
         case GX2AttribFormat::UNORM_8:
            inputElementFormat = DXGI_FORMAT_R8_UNORM; break;
         case GX2AttribFormat::UNORM_8_8:
            inputElementFormat = DXGI_FORMAT_R8G8_UNORM; break;
         case GX2AttribFormat::UNORM_8_8_8_8:
            inputElementFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
         case GX2AttribFormat::UINT_8:
            inputElementFormat = DXGI_FORMAT_R8_UINT; break;
         case GX2AttribFormat::UINT_8_8:
            inputElementFormat = DXGI_FORMAT_R8G8_UINT; break;
         case GX2AttribFormat::UINT_8_8_8_8:
            inputElementFormat = DXGI_FORMAT_R8G8B8A8_UINT; break;
         case GX2AttribFormat::SNORM_8:
            inputElementFormat = DXGI_FORMAT_R8_SNORM; break;
         case GX2AttribFormat::SNORM_8_8:
            inputElementFormat = DXGI_FORMAT_R8G8_SNORM; break;
         case GX2AttribFormat::SNORM_8_8_8_8:
            inputElementFormat = DXGI_FORMAT_R8G8B8A8_SNORM; break;
         case GX2AttribFormat::SINT_8:
            inputElementFormat = DXGI_FORMAT_R8_SINT; break;
         case GX2AttribFormat::SINT_8_8:
            inputElementFormat = DXGI_FORMAT_R8G8_SINT; break;
         case GX2AttribFormat::SINT_8_8_8_8:
            inputElementFormat = DXGI_FORMAT_R8G8B8A8_SINT; break;
         case GX2AttribFormat::FLOAT_32:
            inputElementFormat = DXGI_FORMAT_R32_FLOAT; break;
         case GX2AttribFormat::FLOAT_32_32:
            inputElementFormat = DXGI_FORMAT_R32G32_FLOAT; break;
         case GX2AttribFormat::FLOAT_32_32_32:
            inputElementFormat = DXGI_FORMAT_R32G32B32_FLOAT; break;
         case GX2AttribFormat::FLOAT_32_32_32_32:
            inputElementFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
         default:
            assert(0);
         };

         D3D12_INPUT_ELEMENT_DESC inputElementDesc = {
            "POSITION",
            attrib.location,
            inputElementFormat,
            attrib.buffer,
            attrib.offset,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
         };
         inputElementDescs.emplace_back(inputElementDesc);
      }

      ComPtr<ID3D12PipelineState> pipelineState = nullptr;
      {
         // Describe and create the graphics pipeline state object (PSO).
         D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
         psoDesc.InputLayout = { &inputElementDescs[0], (UINT)inputElementDescs.size() };
         psoDesc.pRootSignature = gDX.rootSignature.Get();
         psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShaderBlob->GetBufferPointer()), vertexShaderBlob->GetBufferSize() };
         psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShaderBlob->GetBufferPointer()), pixelShaderBlob->GetBufferSize() };
         psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
         psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
         psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
         psoDesc.DepthStencilState.DepthEnable = FALSE;
         psoDesc.DepthStencilState.StencilEnable = FALSE;
         psoDesc.SampleMask = UINT_MAX;
         psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
         psoDesc.NumRenderTargets = 1;
         psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
         psoDesc.SampleDesc.Count = 1;

         auto &blendState = gDX.state.blendState;
         if (blendState.blendEnabled) {
            psoDesc.BlendState.IndependentBlendEnable = true;
            for (auto i = 0; i < 8; ++i) {
               auto &targetState = gDX.state.targetBlendState[i];
               auto &rtInfo = psoDesc.BlendState.RenderTarget[i];
               rtInfo.LogicOpEnable = false;
               rtInfo.LogicOp = D3D12_LOGIC_OP_COPY;
               rtInfo.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
               rtInfo.BlendEnable = blendState.blendEnabled & (1 << i);
               rtInfo.SrcBlend = dx12MakeBlend(targetState.colorSrcBlend);
               rtInfo.DestBlend = dx12MakeBlend(targetState.colorDstBlend);
               rtInfo.BlendOp = dx12MakeBlendOp(targetState.colorCombine);
               rtInfo.SrcBlendAlpha = dx12MakeBlend(targetState.alphaSrcBlend);
               rtInfo.DestBlendAlpha = dx12MakeBlend(targetState.alphaDstBlend);
               rtInfo.BlendOpAlpha = dx12MakeBlendOp(targetState.alphaCombine);

            }
         }

         ThrowIfFailed(gDX.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
      }

      auto item = new Item(this);
      item->mFetchShader = fetchShader;
      item->mVertexShader = vertexShader;
      item->mPixelShader = pixelShader;
      item->mPipelineState = pipelineState;
      mItems.push_back(item);
      return item->mPipelineState;
   }

protected:
   std::vector<Item*> mItems;

};
