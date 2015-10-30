#pragma once
#include <memory>
#include <vector>
#include <fstream>
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

      bool matchesCurrentState() {
#define CHECKVAL(valname) \
         if (mStateInfo.valname != gDX.state.valname) { \
            return false; \
         }

         CHECKVAL(fetchShader);
         CHECKVAL(vertexShader);
         CHECKVAL(pixelShader);
         CHECKVAL(blendState.logicOp);
         CHECKVAL(blendState.blendEnabled);
         CHECKVAL(blendState.alphaTestEnabled);
         if (gDX.state.blendState.alphaTestEnabled) {
            CHECKVAL(blendState.alphaFunc);
            CHECKVAL(blendState.alphaRef);
         }

         for (auto j = 0; j < 8; ++j) {
            if (gDX.state.blendState.blendEnabled & (1 << j)) {
               CHECKVAL(targetBlendState[j].colorSrcBlend);
               CHECKVAL(targetBlendState[j].colorDstBlend);
               CHECKVAL(targetBlendState[j].colorCombine);
               CHECKVAL(targetBlendState[j].alphaSrcBlend);
               CHECKVAL(targetBlendState[j].alphaDstBlend);
               CHECKVAL(targetBlendState[j].alphaCombine);
            }
         }

#undef CHECKVAL

         return true;
      }

   protected:
      DXPipelineMgr *mParent;
      struct {
         GX2FetchShader *fetchShader;
         GX2PixelShader *pixelShader;
         GX2VertexShader *vertexShader;
         DXState::ContextState::BlendState blendState;
         DXState::ContextState::TargetBlendState targetBlendState[8];
      } mStateInfo;
      ComPtr<ID3D12PipelineState> mPipelineState;

   };

   typedef std::shared_ptr<Item> ItemPtr;

   DXPipelineMgr()
   {
   }

   ComPtr<ID3D12PipelineState> findOrCreate()
   {
      for (auto &i : mItems) {
         if (!i->matchesCurrentState()) {
            continue;
         }
         return i->mPipelineState;
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
         gLog->trace("Fetch Shader [{}]:\n", (void*)shader);
         gLog->trace("  Type: {}\n", dataPtr->type);
         gLog->trace("  Tess: {}\n", dataPtr->tessMode);
         for (auto i = 0u; i < shader->attribCount; ++i) {
            auto attrib = dataPtr->attribs[i];
            gLog->trace("  Attrib[{}] L:{}, B:{}, O:{}, F:{}, T:{}, A:{}, M:{:08x}, E:{}\n", i,
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
         gLog->trace("Vertex Shader [{}] ({:08x})\n", (void*)shader, vertexShaderCrc);
         GX2DumpShader(shader);
      }
      // Print Pixel Shader Data
      uint32_t pixelShaderCrc;
      {
         auto shader = gDX.state.pixelShader;
         pixelShaderCrc = crc32(shader->data, shader->size);
         gLog->trace("Pixel Shader [{}] ({:08x})\n", (void*)shader, pixelShaderCrc);
         GX2DumpShader(shader);
      }


      auto fetchShader = gDX.state.fetchShader;
      auto vertexShader = gDX.state.vertexShader;
      auto pixelShader = gDX.state.pixelShader;
      auto fetchData = (FetchShaderInfo*)(void*)fetchShader->data;

      std::string hlsl;
      {
         latte::Shader decVertexShader, decPixelShader;
         latte::decode(decVertexShader, latte::Shader::Vertex, { vertexShader->data.get(), vertexShader->size });
         latte::decode(decPixelShader, latte::Shader::Pixel, { pixelShader->data.get(), pixelShader->size });

         std::string psAddend = "";
         if (gDX.state.blendState.alphaTestEnabled) {
            auto alphaFunc = gDX.state.blendState.alphaFunc;
            if (alphaFunc == GX2CompareFunction::Always) {
               psAddend = "discard;\n";
            } else if (alphaFunc == GX2CompareFunction::Never) {
               psAddend = "";
            } else {
               const char *testFunc = "";
               switch (alphaFunc) {
               case GX2CompareFunction::Less: testFunc = "<"; break;
               case GX2CompareFunction::Equal: testFunc = "=="; break;
               case GX2CompareFunction::LessOrEqual: testFunc = "<="; break;
               case GX2CompareFunction::Greater: testFunc = ">"; break;
               case GX2CompareFunction::NotEqual: testFunc = "!="; break;
               case GX2CompareFunction::GreaterOrEqual: testFunc = ">="; break;
               }
               auto alphaRef = gDX.state.blendState.alphaRef;
               psAddend = fmt::format("if (result.color0.a {} {}) discard;\n", testFunc, alphaRef);
            }
         }

         hlsl::generateHLSL({ (GX2AttribStream*)fetchData->attribs, fetchShader->attribCount },
                            vertexShader, decVertexShader,
                            pixelShader, decPixelShader,
                            "", psAddend,
                            hlsl);

         gLog->trace("Compiled Shader:\n{}\n", hlsl);
      }

      ComPtr<ID3DBlob> vertexShaderBlob;
      ComPtr<ID3DBlob> pixelShaderBlob;

      // Enable better shader debugging with the graphics debugging tools.
      UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

      std::string hlslFileName = fmt::format("dump/shader_{:08x}_{:08x}_{:08x}.hlsl",
         memory_untranslate(fetchShader),
         memory_untranslate(vertexShader),
         memory_untranslate(pixelShader));
      std::wstring hlslFileNameW(hlslFileName.begin(), hlslFileName.end());
      {
         auto file = std::ofstream{ hlslFileNameW, std::ofstream::out | std::ofstream::binary };
         file.write(hlsl.c_str(), hlsl.size());
      }

      ComPtr<ID3DBlob> errorBlob;
      HRESULT vertHr, pixHr;
      vertHr = D3DCompileFromFile(hlslFileNameW.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShaderBlob, &errorBlob);
      if (!FAILED(vertHr)) {
         pixHr = D3DCompileFromFile(hlslFileNameW.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShaderBlob, &errorBlob);
      }

      if (FAILED(vertHr) || FAILED(pixHr)) {
         if (FAILED(vertHr)) {
            gLog->warn("Vertex Shader Compilation Failed");
         } else {
            gLog->warn("Pixel Shader Compilation Failed");
         }

         gLog->warn("Error: {}\n", (char*)errorBlob->GetBufferPointer());
         gLog->warn("Shader Source:\n{}\n", hlsl);

         ThrowIfFailed(D3DCompileFromFile(L"resources/shaders/wiiu_fallback.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShaderBlob, nullptr));
         ThrowIfFailed(D3DCompileFromFile(L"resources/shaders/wiiu_fallback.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShaderBlob, nullptr));
      }

      DXGI_FORMAT inputElementFormat;
      std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
      for (auto i = 0u; i < fetchShader->attribCount; ++i) {
         auto &attrib = fetchData->attribs[i];

         if (attrib.type != 0) {
            throw;
         }

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
            throw;
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
         psoDesc.RasterizerState.DepthClipEnable = FALSE;
         psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
         psoDesc.DepthStencilState.DepthEnable = FALSE;
         psoDesc.DepthStencilState.StencilEnable = FALSE;
         psoDesc.SampleMask = UINT_MAX;
         psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
         psoDesc.NumRenderTargets = 1;
         psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
         psoDesc.SampleDesc.Count = 1;

         if (gDX.state.primitiveRestartIdx == 0xFFFF) {
            psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
         } else if (gDX.state.primitiveRestartIdx == 0xFFFFFFFF) {
            psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
         } else {
            throw;
         }

         auto &blendState = gDX.state.blendState;
         if (blendState.blendEnabled == 0) {
            // Cannot use logicOp with blendEnabled set to anything but 0,
            //   and with DX12, you need to have IndependantBlendOff for
            //   LogicOp specific things as well.
            psoDesc.BlendState.IndependentBlendEnable = false;
         } else {
            psoDesc.BlendState.IndependentBlendEnable = true;
         }
         for (auto i = 0; i < 8; ++i) {
            auto &targetState = gDX.state.targetBlendState[i];
            auto &rtInfo = psoDesc.BlendState.RenderTarget[i];
            rtInfo.BlendEnable = blendState.blendEnabled & (1 << i);
            rtInfo.LogicOpEnable = !rtInfo.BlendEnable;
            rtInfo.LogicOp = dx12MakeLogicOp(blendState.logicOp);
            rtInfo.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            rtInfo.SrcBlend = dx12MakeBlend(targetState.colorSrcBlend);
            rtInfo.DestBlend = dx12MakeBlend(targetState.colorDstBlend);
            rtInfo.BlendOp = dx12MakeBlendOp(targetState.colorCombine);
            rtInfo.SrcBlendAlpha = dx12MakeBlend(targetState.alphaSrcBlend);
            rtInfo.DestBlendAlpha = dx12MakeBlend(targetState.alphaDstBlend);
            rtInfo.BlendOpAlpha = dx12MakeBlendOp(targetState.alphaCombine);
         }

         ThrowIfFailed(gDX.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
      }

      auto item = new Item(this);
      item->mStateInfo.fetchShader = fetchShader;
      item->mStateInfo.vertexShader = vertexShader;
      item->mStateInfo.pixelShader = pixelShader;
      memcpy(&item->mStateInfo.blendState, &gDX.state.blendState, sizeof(DXState::ContextState::BlendState));
      memcpy(&item->mStateInfo.targetBlendState, &gDX.state.targetBlendState, sizeof(DXState::ContextState::TargetBlendState) * 8);
      item->mPipelineState = pipelineState;
      mItems.push_back(item);
      return item->mPipelineState;
   }

protected:
   std::vector<Item*> mItems;

};
