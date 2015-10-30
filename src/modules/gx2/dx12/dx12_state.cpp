#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include <memory>
#include "config.h"
#include "hostlookup.h"
#include "dx12_state.h"
#include "dx12_scanbuffer.h"
#include "dx12_colorbuffer.h"
#include "dx12_depthbuffer.h"
#include "dx12_pipelinemgr.h"
#include "dx12_texture.h"
#include "platform/platform_ui.h"
#include "utils/byte_swap.h"
#include "../gx2_sampler.h"

DXState gDX;

HostLookupTable<DXColorBufferData, GX2ColorBuffer> colorBufferLkp;
HostLookupTable<DXDepthBufferData, GX2DepthBuffer> depthBufferLkp;
HostLookupTable<DXTextureData, GX2Texture> textureLkp;

DXColorBufferData * dx::getColorBuffer(GX2ColorBuffer *buffer) {
   return colorBufferLkp.get(buffer);
}

DXDepthBufferData * dx::getDepthBuffer(GX2DepthBuffer *buffer) {
   return depthBufferLkp.get(buffer);
}

DXTextureData * dx::getTexture(GX2Texture *buffer) {
   return textureLkp.get(buffer);
}

void dx::initialise()
{
   gDX.activeContextState = nullptr;
   for (auto i = 0; i < 3; ++i) {
      auto &displayList = gDX.activeDisplayList[i];
      displayList.buffer = nullptr;
      displayList.size = 0;
      displayList.offset = 0;
   }
   for (auto i = 0; i < GX2_NUM_MRT_BUFFER; ++i) {
      gDX.activeColorBuffer[i] = nullptr;
   }
   gDX.activeDepthBuffer = nullptr;
   for (auto i = 0; i < GX2_NUM_SAMPLERS; ++i) {
      gDX.activeTextures[i] = nullptr;
   }

   gDX.state.primitiveRestartIdx = 0xFFFFFFFF;
   gDX.state.shaderMode = GX2ShaderMode::UniformRegister;
   for (auto i = 0; i < 16; ++i) {
      gDX.state.pixUniformBlocks[i].buffer = nullptr;
      gDX.state.pixUniformBlocks[i].size = 0;
      gDX.state.vertUniformBlocks[i].buffer = nullptr;
      gDX.state.vertUniformBlocks[i].size = 0;
   }

   gDX.viewport.Width = static_cast<float>(platform::ui::getWindowWidth());
   gDX.viewport.Height = static_cast<float>(platform::ui::getWindowHeight());
   gDX.viewport.MaxDepth = 1.0f;

   gDX.scissorRect.right = static_cast<LONG>(platform::ui::getWindowWidth());
   gDX.scissorRect.bottom = static_cast<LONG>(platform::ui::getWindowHeight());

   // Enable the D3D12 debug layer.
   {
      ComPtr<ID3D12Debug> debugController;
      if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
      {
         debugController->EnableDebugLayer();
      }
   }

   ComPtr<IDXGIFactory4> factory;
   ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

   // Create adapter (set use_warp for software renderer)
   if (config::dx12::use_warp) {
      ComPtr<IDXGIAdapter> warpAdapter;
      ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

      ThrowIfFailed(D3D12CreateDevice(
         warpAdapter.Get(),
         D3D_FEATURE_LEVEL_11_0,
         IID_PPV_ARGS(&gDX.device)
         ));
   } else {
      ThrowIfFailed(D3D12CreateDevice(
         nullptr,
         D3D_FEATURE_LEVEL_11_0,
         IID_PPV_ARGS(&gDX.device)
         ));
   }

   // Describe and create the command queue.
   D3D12_COMMAND_QUEUE_DESC queueDesc = {};
   queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
   queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

   ThrowIfFailed(gDX.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&gDX.commandQueue)));

   // Describe and create the swap chain.
   DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
   swapChainDesc.BufferCount = gDX.FrameCount;
   swapChainDesc.BufferDesc.Width = platform::ui::getWindowWidth();
   swapChainDesc.BufferDesc.Height = platform::ui::getWindowHeight();
   swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   swapChainDesc.OutputWindow = reinterpret_cast<HWND>(platform::ui::getWindowHandle());
   swapChainDesc.SampleDesc.Count = 1;
   swapChainDesc.Windowed = TRUE;

   ComPtr<IDXGISwapChain> swapChain;
   ThrowIfFailed(factory->CreateSwapChain(
      gDX.commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
      &swapChainDesc,
      &swapChain
      ));

   ThrowIfFailed(swapChain.As(&gDX.swapChain));

   gDX.frameIndex = gDX.swapChain->GetCurrentBackBufferIndex();

   // Create descriptor heaps.
   {
      // Describe and create a render target view (RTV) descriptor heap.
      D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
      rtvHeapDesc.NumDescriptors = gDX.FrameCount + 128;
      rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
      rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
      gDX.rtvHeap = new DXHeap(gDX.device.Get(), rtvHeapDesc);

      // Describe and create a depth stencil view (DSV) descriptor heap.
      D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
      dsvHeapDesc.NumDescriptors = 128;
      dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
      dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
      gDX.dsvHeap = new DXHeap(gDX.device.Get(), dsvHeapDesc);

      // Describe and create a constant buffer view (CBV), Shader resource
      // view (SRV), and unordered access view (UAV) descriptor heap.
      D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
      srvHeapDesc.NumDescriptors = 2048;
      srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      gDX.srvHeap = new DXHeap(gDX.device.Get(), srvHeapDesc);

      // Create a RTV for each frame.
      for (UINT n = 0; n < gDX.FrameCount; n++)
      {
         D3D12_DESCRIPTOR_HEAP_DESC srvHeapListDesc = {};
         srvHeapListDesc.NumDescriptors = 2048;
         srvHeapListDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
         srvHeapListDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
         gDX.srvHeapList[n] = new DXHeapList(gDX.device.Get(), srvHeapListDesc);

         D3D12_DESCRIPTOR_HEAP_DESC sampleHeapListDesc = {};
         sampleHeapListDesc.NumDescriptors = 2048;
         sampleHeapListDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
         sampleHeapListDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
         gDX.sampleHeapList[n] = new DXHeapList(gDX.device.Get(), sampleHeapListDesc);
      }
   }

   // Create frame resources.
   {
      // Create a RTV for each frame.
      for (UINT n = 0; n < gDX.FrameCount; n++)
      {
         ThrowIfFailed(gDX.swapChain->GetBuffer(n, IID_PPV_ARGS(&gDX.renderTargets[n])));

         gDX.scanbufferRtv[n] = gDX.rtvHeap->alloc();
         gDX.device->CreateRenderTargetView(gDX.renderTargets[n].Get(), nullptr, *gDX.scanbufferRtv[n]);
      }
   }

   // Create command allocators.
   {
      for (UINT n = 0; n < gDX.FrameCount; n++)
      {
         ThrowIfFailed(gDX.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gDX.commandAllocator[n])));
      }
   }

   // Create the root signature.
   {

      CD3DX12_ROOT_PARAMETER rootParameters[3];
      CD3DX12_DESCRIPTOR_RANGE ranges[3];
      uint32_t paramIdx = 0;

      ranges[paramIdx].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, GX2_NUM_UNIFORMBLOCKS, 0);
      rootParameters[paramIdx].InitAsDescriptorTable(1, &ranges[paramIdx], D3D12_SHADER_VISIBILITY_ALL);
      paramIdx++;

      ranges[paramIdx].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GX2_NUM_SAMPLERS, 0);
      rootParameters[paramIdx].InitAsDescriptorTable(1, &ranges[paramIdx], D3D12_SHADER_VISIBILITY_ALL);
      paramIdx++;

      ranges[paramIdx].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, GX2_NUM_SAMPLERS, 0);
      rootParameters[paramIdx].InitAsDescriptorTable(1, &ranges[paramIdx], D3D12_SHADER_VISIBILITY_ALL);
      paramIdx++;

      CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
      rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

      ComPtr<ID3DBlob> signature;
      ComPtr<ID3DBlob> error;
      ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
      ThrowIfFailed(gDX.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&gDX.rootSignature)));
   }

   // Create the pipeline state, which includes compiling and loading shaders.
   {
      ComPtr<ID3DBlob> vertexShader;
      ComPtr<ID3DBlob> pixelShader;

      // Enable better shader debugging with the graphics debugging tools.
      UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

      ThrowIfFailed(D3DCompileFromFile(L"resources/shaders/screendraw.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
      ThrowIfFailed(D3DCompileFromFile(L"resources/shaders/screendraw.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

      // Define the vertex input layout.
      D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
      {
         { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
         { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
      };

      {
         D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
         psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
         psoDesc.pRootSignature = gDX.rootSignature.Get();
         psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
         psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
         psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
         psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
         psoDesc.DepthStencilState.DepthEnable = FALSE;
         psoDesc.DepthStencilState.StencilEnable = FALSE;
         psoDesc.SampleMask = UINT_MAX;
         psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
         psoDesc.NumRenderTargets = 1;
         psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
         psoDesc.SampleDesc.Count = 1;
         ThrowIfFailed(gDX.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&gDX.emuPipelineState)));
      }
   }

   gDX.pipelineMgr = new DXPipelineMgr();

   // Create the command list.
   gDX.frameIndex = gDX.swapChain->GetCurrentBackBufferIndex();

   ThrowIfFailed(gDX.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gDX.commandAllocator[gDX.frameIndex].Get(), gDX.emuPipelineState.Get(), IID_PPV_ARGS(&gDX.commandList)));



   {
#define SCREENSPACE(x, y) { -1 + ((x) / (float)platform::ui::getWindowWidth()) * 2, 1 - ((y) / (float)platform::ui::getWindowHeight()) * 2, 0.0f }
      float tvX = 0;
      float tvY = 0;
      float tvWidth = static_cast<float>(platform::ui::getTvWidth());
      float tvHeight = static_cast<float>(platform::ui::getTvHeight());
      float drcX = (tvWidth - static_cast<float>(platform::ui::getDrcWidth())) / 2.0f;
      float drcY = tvHeight;
      float drcWidth = static_cast<float>(platform::ui::getDrcWidth());
      float drcHeight = static_cast<float>(platform::ui::getDrcHeight());

      struct Vertex {
         XMFLOAT3 pos;
         XMFLOAT2 uv;
      } triangleVertices[] =
      {
         { SCREENSPACE(tvX, tvY + tvHeight),{ 0.0f, 1.0f } },
         { SCREENSPACE(tvX, tvY),{ 0.0f, 0.0f } },
         { SCREENSPACE(tvX + tvWidth, tvY + tvHeight),{ 1.0f, 1.0f } },
         { SCREENSPACE(tvX + tvWidth, tvY),{ 1.0f, 0.0f } },
         { SCREENSPACE(drcX, drcY + drcHeight),{ 0.0f, 1.0f } },
         { SCREENSPACE(drcX, drcY),{ 0.0f, 0.0f } },
         { SCREENSPACE(drcX + drcWidth, drcY + drcHeight),{ 1.0f, 1.0f } },
         { SCREENSPACE(drcX + drcWidth, drcY),{ 1.0f, 0.0f } },
      };

#undef SCREENSPACE

      const UINT vertexBufferSize = sizeof(triangleVertices);

      // Note: using upload heaps to transfer static data like vert buffers is not
      // recommended. Every time the GPU needs it, the upload heap will be marshalled
      // over. Please read up on Default Heap usage. An upload heap is used here for
      // code simplicity and because there are very few verts to actually transfer.
      ThrowIfFailed(gDX.device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
         D3D12_RESOURCE_STATE_GENERIC_READ,
         nullptr,
         IID_PPV_ARGS(&gDX.vertexBuffer)));

      // Copy the triangle data to the vertex buffer.
      UINT8* pVertexDataBegin;
      CD3DX12_RANGE readRange(0, 0);
      ThrowIfFailed(gDX.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
      memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
      gDX.vertexBuffer->Unmap(0, nullptr);

      // Initialize the vertex buffer view.
      gDX.vertexBufferView.BufferLocation = gDX.vertexBuffer->GetGPUVirtualAddress();
      gDX.vertexBufferView.StrideInBytes = sizeof(Vertex);
      gDX.vertexBufferView.SizeInBytes = vertexBufferSize;
   }

   for (UINT n = 0; n < gDX.FrameCount; n++)
   {
      // 10MB Temporary Buffer
      gDX.tmpBuffer[n] = new DXDynBuffer(gDX.device.Get(), 10 * 1024 * 1024);
   }

   // Close the command list and execute it to begin the initial GPU setup.
   ThrowIfFailed(gDX.commandList->Close());
   ID3D12CommandList* ppCommandLists[] = { gDX.commandList.Get() };
   gDX.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

   // Create synchronization objects.
   {
      ThrowIfFailed(gDX.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gDX.fence)));

      // Create an event handle to use for frame synchronization.
      gDX.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
      if (gDX.fenceEvent == nullptr)
      {
         ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
      }

      // Wait for frame completion
      gDX.swapCount++;
      const uint64_t fenceValue = gDX.swapCount;
      ThrowIfFailed(gDX.commandQueue->Signal(gDX.fence.Get(), fenceValue));

      if (gDX.fence->GetCompletedValue() < gDX.swapCount)
      {
         ThrowIfFailed(gDX.fence->SetEventOnCompletion(gDX.swapCount, gDX.fenceEvent));
         WaitForSingleObject(gDX.fenceEvent, INFINITE);
      }
   }

   _beginFrame();
}

void dx::renderScanBuffers() {
   _endFrame();
   _beginFrame();
}

void dx::_beginFrame() {
   gDX.frameIndex = gDX.swapChain->GetCurrentBackBufferIndex();
   gDX.curScanbufferRtv = gDX.scanbufferRtv[gDX.frameIndex];
   gDX.curSrvHeapList = gDX.srvHeapList[gDX.frameIndex];
   gDX.curSampleHeapList = gDX.sampleHeapList[gDX.frameIndex];
   gDX.curTmpBuffer = gDX.tmpBuffer[gDX.frameIndex];
   gDX.curSrvHeapList->reset();
   gDX.curSampleHeapList->reset();
   gDX.curTmpBuffer->reset();

   // Command list allocators can only be reset when the associated
   // command lists have finished execution on the GPU; apps should use
   // fences to determine GPU execution progress.
   ThrowIfFailed(gDX.commandAllocator[gDX.frameIndex]->Reset());

   // However, when ExecuteCommandList() is called on a particular command
   // list, that command list can then be reset at any time and must be before
   // re-recording.
   ThrowIfFailed(gDX.commandList->Reset(gDX.commandAllocator[gDX.frameIndex].Get(), gDX.emuPipelineState.Get()));

   // Set necessary state.
   gDX.commandList->SetGraphicsRootSignature(gDX.rootSignature.Get());

   ID3D12DescriptorHeap* ppHeaps[] = {
      (ID3D12DescriptorHeap*)(*gDX.curSrvHeapList),
      (ID3D12DescriptorHeap*)(*gDX.curSampleHeapList)
   };
   gDX.commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

   const float clearColor[] = { 0.6f, 0.2f, 0.2f, 1.0f };
   gDX.commandList->ClearRenderTargetView(*gDX.curScanbufferRtv, clearColor, 0, nullptr);
}

void dx::_endFrame() {
   gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gDX.renderTargets[gDX.frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

   gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gDX.tvScanBuffer->buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
   gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gDX.drcScanBuffer->buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

   gDX.commandList->SetPipelineState(gDX.emuPipelineState.Get());

   gDX.commandList->RSSetViewports(1, &gDX.viewport);
   gDX.commandList->RSSetScissorRects(1, &gDX.scissorRect);

   gDX.commandList->OMSetRenderTargets(1, *gDX.curScanbufferRtv, FALSE, nullptr);

   gDX.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
   gDX.commandList->IASetVertexBuffers(0, 1, &gDX.vertexBufferView);

   // Setup Samplers
   {
      DXHeapList::Item sampler = gDX.curSampleHeapList->alloc();
      for (auto i = 0; i < GX2_NUM_SAMPLERS - 1; ++i) {
         gDX.curSampleHeapList->alloc();
      }

      D3D12_SAMPLER_DESC samplerDesc;
      samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      samplerDesc.MipLODBias = 0;
      samplerDesc.MaxAnisotropy = 0;
      samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
      samplerDesc.BorderColor[0] = 0;
      samplerDesc.BorderColor[1] = 0;
      samplerDesc.BorderColor[2] = 0;
      samplerDesc.BorderColor[3] = 0;
      samplerDesc.MinLOD = 0.0f;
      samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
      gDX.device->CreateSampler(&samplerDesc, sampler);

      gDX.commandList->SetGraphicsRootDescriptorTable(2, sampler);
   }

   // Render TV ScanBuffer
   {
      DXHeapList::Item srv = gDX.curSrvHeapList->alloc();
      for (auto i = 0; i < GX2_NUM_SAMPLERS - 1; ++i) {
         gDX.curSrvHeapList->alloc();
      }
      gDX.device->CreateShaderResourceView(*gDX.tvScanBuffer, *gDX.tvScanBuffer, srv);

      gDX.commandList->SetGraphicsRootDescriptorTable(1, srv);
      gDX.commandList->DrawInstanced(4, 1, 0, 0);
   }

   // Render DRC ScanBuffer
   {
      DXHeapList::Item srv = gDX.curSrvHeapList->alloc();
      for (auto i = 0; i < GX2_NUM_SAMPLERS - 1; ++i) {
         gDX.curSrvHeapList->alloc();
      }
      gDX.device->CreateShaderResourceView(*gDX.drcScanBuffer, *gDX.drcScanBuffer, srv);

      gDX.commandList->SetGraphicsRootDescriptorTable(1, srv);
      gDX.commandList->DrawInstanced(4, 1, 4, 0);
   }

   gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gDX.tvScanBuffer->buffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
   gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gDX.drcScanBuffer->buffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

   gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gDX.renderTargets[gDX.frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

   // Finish the command list
   ThrowIfFailed(gDX.commandList->Close());

   // Execute the command list.
   ID3D12CommandList* ppCommandLists[] = { gDX.commandList.Get() };
   gDX.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

   // Present the frame.
   ThrowIfFailed(gDX.swapChain->Present(1, 0));

   // Increment the counters
   gDX.swapCount++;
   const uint64_t fenceValue = gDX.swapCount;
   ThrowIfFailed(gDX.commandQueue->Signal(gDX.fence.Get(), fenceValue));
}

void dx::updateRenderTargets()
{
   bool needsUpdate = false;
   for (auto i = 0; i < GX2_NUM_MRT_BUFFER; ++i) {
      if (gDX.activeColorBuffer[i] != gDX.state.colorBuffer[i]) {
         needsUpdate = true;
         break;
      }
   }
   if (gDX.activeDepthBuffer != gDX.state.depthBuffer) {
      needsUpdate = true;
   }

   if (!needsUpdate) {
      return;
   }

   D3D12_CPU_DESCRIPTOR_HANDLE buffers[GX2_NUM_MRT_BUFFER];
   int numBuffers = 0;

   for (auto i = 0; i < GX2_NUM_MRT_BUFFER; ++i) {
      auto buffer = gDX.state.colorBuffer[i];
      if (!buffer) {
         break;
      }

      auto hostColorBuffer = dx::getColorBuffer(buffer);
      buffers[i] = *hostColorBuffer->rtv;
      numBuffers++;
   }

   D3D12_CPU_DESCRIPTOR_HANDLE *depthBuffer = nullptr;
   if (gDX.state.depthBuffer) {
      auto hostDepthBuffer = dx::getDepthBuffer(gDX.state.depthBuffer);
      depthBuffer = *hostDepthBuffer->dsv;
   }

   gDX.commandList->OMSetRenderTargets(numBuffers, buffers, FALSE, depthBuffer);

   // Store active RT's
   for (auto i = 0; i < GX2_NUM_MRT_BUFFER; ++i) {
      gDX.activeColorBuffer[i] = gDX.state.colorBuffer[i];
   }
   gDX.activeDepthBuffer = gDX.state.depthBuffer;
}

void dx::updatePipeline()
{
   auto pipelineState = gDX.pipelineMgr->findOrCreate().Get();
   gDX.commandList->SetPipelineState(pipelineState);

   gDX.commandList->OMSetBlendFactor(gDX.state.blendState.constColor);
}

template<typename Type, int N, bool EndianSwap>
void stridedMemcpy3(uint8_t *src, uint8_t *dest, size_t size, uint32_t stride, uint32_t offset) {
   uint32_t extraStride = stride - (sizeof(Type) * N);
   Type *s = (Type*)(src + offset);
   Type *d = (Type*)(dest + offset);
   Type *sEnd = (Type*)(src + size);
   while (s < sEnd) {
      if (EndianSwap) {
         for (auto i = 0u; i < N; ++i) {
            *d++ = byte_swap(*s++);
         }
         *((uint8_t**)&s) += extraStride;
         *((uint8_t**)&d) += extraStride;
      } else {
         memcpy(s, d, sizeof(Type) * N);
         *((uint8_t**)&s) += stride;
         *((uint8_t**)&d) += stride;
      }
   }
}

template<typename Type, int N>
void stridedMemcpy2(
   uint8_t *src, uint8_t *dest, size_t size,
   uint32_t stride, uint32_t offset,
   GX2EndianSwapMode::Value endianess) {
   if (endianess == GX2EndianSwapMode::Default || endianess == GX2EndianSwapMode::None) {
      return stridedMemcpy3<Type, N, true>(src, dest, size, stride, offset);
   } else {
      return stridedMemcpy3<Type, N, false>(src, dest, size, stride, offset);
   }
}

void stridedMemcpy(
   uint8_t *src, uint8_t *dest, size_t size,
   uint32_t stride, uint32_t offset,
   GX2EndianSwapMode::Value endianess, GX2AttribFormat::Value format) {
   switch (format) {
   case GX2AttribFormat::UNORM_8:
      return stridedMemcpy2<uint8_t, 1>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::UNORM_8_8:
      return stridedMemcpy2<uint8_t, 2>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::UNORM_8_8_8_8:
      return stridedMemcpy2<uint8_t, 4>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::UINT_8:
      return stridedMemcpy2<uint8_t, 1>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::UINT_8_8:
      return stridedMemcpy2<uint8_t, 2>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::UINT_8_8_8_8:
      return stridedMemcpy2<uint8_t, 4>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::SNORM_8:
      return stridedMemcpy2<uint8_t, 1>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::SNORM_8_8:
      return stridedMemcpy2<uint8_t, 2>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::SNORM_8_8_8_8:
      return stridedMemcpy2<uint8_t, 4>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::SINT_8:
      return stridedMemcpy2<int8_t, 1>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::SINT_8_8:
      return stridedMemcpy2<int8_t, 2>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::SINT_8_8_8_8:
      return stridedMemcpy2<int8_t, 4>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::FLOAT_32:
      return stridedMemcpy2<float, 1>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::FLOAT_32_32:
      return stridedMemcpy2<float, 2>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::FLOAT_32_32_32:
      return stridedMemcpy2<float, 3>(src, dest, size, stride, offset, endianess);
   case GX2AttribFormat::FLOAT_32_32_32_32:
      return stridedMemcpy2<float, 4>(src, dest, size, stride, offset, endianess);
   default:
      throw;
   };
}

void dx::updateBuffers()
{
   DXDynBuffer::VertexAllocation buffers[32];
   D3D12_VERTEX_BUFFER_VIEW bufferList[32];
   auto fetchShader = gDX.state.fetchShader;
   auto fetchData = (FetchShaderInfo*)(void*)fetchShader->data;

   for (auto i = 0; i < 32; ++i) {
      auto& buffer = gDX.state.attribBuffers[i];
      if (!buffer.buffer || !buffer.size) {
         bufferList[i].BufferLocation = NULL;
         bufferList[i].SizeInBytes = 0;
         bufferList[i].StrideInBytes = 0;
         continue;
      }

      buffers[i] = gDX.curTmpBuffer->get(
         buffer.stride, buffer.size, nullptr, 256);
      bufferList[i] = *(D3D12_VERTEX_BUFFER_VIEW*)buffers[i];
   }

   for (auto i = 0u; i < fetchShader->attribCount; ++i) {
      auto attrib = fetchData->attribs[i];
      auto srcBuffer = gDX.state.attribBuffers[attrib.buffer];
      auto destBuffer = buffers[attrib.buffer];

      if (!destBuffer.valid()) {
         throw;
      }

      stridedMemcpy(
         (uint8_t*)srcBuffer.buffer,
         (uint8_t*)destBuffer,
         srcBuffer.size,
         srcBuffer.stride,
         attrib.offset,
         attrib.endianSwap,
         attrib.format);
   }

   for (auto i = 0; i < 32; ++i) {
      if (bufferList[i].BufferLocation) {
         gDX.commandList->IASetVertexBuffers(i, 1, &bufferList[i]);
      }
   }

   {
      DXHeapList::Item heapItems[GX2_NUM_UNIFORMBLOCKS];
      uint32_t numActiveCbvs = 0;

      if (gDX.state.shaderMode == GX2ShaderMode::UniformRegister) {
         numActiveCbvs = 2;
         for (auto i = 0u; i < numActiveCbvs; ++i) {
            heapItems[i] = gDX.curSrvHeapList->alloc();
         }

         {
            auto constBuffer = gDX.curTmpBuffer->get(GX2_NUM_GPRS * 4 * sizeof(float), gDX.state.vertUniforms, 256);

            D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
            desc.BufferLocation = constBuffer;
            desc.SizeInBytes = GX2_NUM_GPRS * 4 * sizeof(float);
            gDX.device->CreateConstantBufferView(&desc, heapItems[0]);
         }
         {
            auto constBuffer = gDX.curTmpBuffer->get(GX2_NUM_GPRS * 4 * sizeof(float), gDX.state.pixUniforms, 256);

            D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
            desc.BufferLocation = constBuffer;
            desc.SizeInBytes = GX2_NUM_GPRS * 4 * sizeof(float);
            gDX.device->CreateConstantBufferView(&desc, heapItems[1]);
         }
      } else if (gDX.state.shaderMode == GX2ShaderMode::UniformBlock) {
         numActiveCbvs = gDX.state.vertexShader->uniformBlockCount + gDX.state.pixelShader->uniformBlockCount;
         for (auto i = 0u; i < numActiveCbvs; ++i) {
            heapItems[i] = gDX.curSrvHeapList->alloc();
         }

         uint32_t cbvIdx = 0;
         if (gDX.state.vertexShader != nullptr) {
            for (auto i = 0u; i < gDX.state.vertexShader->uniformBlockCount; ++i) {
               auto &uniBlock = gDX.state.vertexShader->uniformBlocks[i];
               auto &blockData = gDX.state.vertUniformBlocks[uniBlock.offset];

               uint32_t alignedSize = align_up(blockData.size, 256);
               auto constBuffer = gDX.curTmpBuffer->get(alignedSize, nullptr, 256);
               memcpy((uint8_t*)constBuffer, blockData.buffer, blockData.size);

               D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
               desc.BufferLocation = constBuffer;
               desc.SizeInBytes = alignedSize;
               gDX.device->CreateConstantBufferView(&desc, heapItems[cbvIdx++]);
            }
         }
         if (gDX.state.pixelShader != nullptr) {
            for (auto i = 0u; i < gDX.state.pixelShader->uniformBlockCount; ++i) {
               auto &uniBlock = gDX.state.pixelShader->uniformBlocks[i];
               auto &blockData = gDX.state.pixUniformBlocks[uniBlock.offset];

               uint32_t alignedSize = align_up(blockData.size, 256);
               auto constBuffer = gDX.curTmpBuffer->get(alignedSize, nullptr, 256);
               memcpy((uint8_t*)constBuffer, blockData.buffer, blockData.size);

               D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
               desc.BufferLocation = constBuffer;
               desc.SizeInBytes = alignedSize;
               gDX.device->CreateConstantBufferView(&desc, heapItems[cbvIdx++]);
            }
         }
      } else {
         // We don't support Geometry shaders just yet...
         throw;
      }

      if (numActiveCbvs) {
         gDX.commandList->SetGraphicsRootDescriptorTable(0, heapItems[0]);
      }
   }
   {
      DXHeapList::Item heapItems[GX2_NUM_SAMPLERS];
      DXHeapList::Item samplerItems[GX2_NUM_SAMPLERS];

      uint32_t numActiveTex = 0;
      for (auto i = 0; i < GX2_NUM_SAMPLERS; ++i) {
         if (gDX.activeTextures[i]) {
            numActiveTex = i + 1;
         }
      }
      for (auto i = 0u; i < numActiveTex; ++i) {
         heapItems[i] = gDX.curSrvHeapList->alloc();
         samplerItems[i] = gDX.curSampleHeapList->alloc();
      }

      for (auto i = 0; i < GX2_NUM_SAMPLERS; ++i) {
         auto &tex = gDX.activeTextures[i];
         auto &fetch = gDX.state.pixelSampler[i];
         if (tex) {
            gDX.device->CreateShaderResourceView(*tex, *tex, heapItems[i]);

            D3D12_SAMPLER_DESC samplerDesc;
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.MipLODBias = 0;
            samplerDesc.MaxAnisotropy = 0;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            samplerDesc.BorderColor[0] = 0;
            samplerDesc.BorderColor[1] = 0;
            samplerDesc.BorderColor[2] = 0;
            samplerDesc.BorderColor[3] = 0;
            samplerDesc.MinLOD = 0.0f;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            if (fetch) {
               samplerDesc.Filter = D3D12_ENCODE_BASIC_FILTER(
                  dx12MakeFilterType(fetch->minFilter),
                  dx12MakeFilterType(fetch->magFilter),
                  dx12MakeMipFilterType(fetch->filterMip),
                  D3D12_FILTER_REDUCTION_TYPE_STANDARD);
               if (fetch->filterMip == GX2TexMipFilterMode::None) {
                  samplerDesc.MinLOD = 0.0f;
                  samplerDesc.MaxLOD = 0.0f;
               }

               samplerDesc.AddressU = dx12MakeAddressMode(fetch->clampX);
               samplerDesc.AddressV = dx12MakeAddressMode(fetch->clampY);
               samplerDesc.AddressW = dx12MakeAddressMode(fetch->clampZ);
               switch (fetch->borderType) {
               case GX2TexBorderType::TransparentBlack:
                  samplerDesc.BorderColor[0] = 0;
                  samplerDesc.BorderColor[1] = 0;
                  samplerDesc.BorderColor[2] = 0;
                  samplerDesc.BorderColor[3] = 0;
                  break;
               case GX2TexBorderType::Black:
                  samplerDesc.BorderColor[0] = 0;
                  samplerDesc.BorderColor[1] = 0;
                  samplerDesc.BorderColor[2] = 0;
                  samplerDesc.BorderColor[3] = 1;
                  break;
               case GX2TexBorderType::White:
                  samplerDesc.BorderColor[0] = 1;
                  samplerDesc.BorderColor[1] = 1;
                  samplerDesc.BorderColor[2] = 1;
                  samplerDesc.BorderColor[3] = 1;
                  break;
               case GX2TexBorderType::Variable:
                  samplerDesc.BorderColor[0] = gDX.state.pixelSamplerBorder[i][0];
                  samplerDesc.BorderColor[1] = gDX.state.pixelSamplerBorder[i][1];
                  samplerDesc.BorderColor[2] = gDX.state.pixelSamplerBorder[i][2];
                  samplerDesc.BorderColor[3] = gDX.state.pixelSamplerBorder[i][3];
                  break;
               }
            }
            gDX.device->CreateSampler(&samplerDesc, samplerItems[i]);
         }
      }

      if (numActiveTex > 0) {
         gDX.commandList->SetGraphicsRootDescriptorTable(1, heapItems[0]);
         gDX.commandList->SetGraphicsRootDescriptorTable(2, samplerItems[0]);
      }
   }
}

void dx::updateTextures()
{
   // We actually do this live right now...
}

#endif
