#include "../gx2.h"
#ifdef GX2_DX12

#include <memory>
#include "systemtypes.h"
#include "hostlookup.h"
#include "platform.h"
#include "dx12_state.h"
#include "dx12_scanbuffer.h"
#include "dx12_colorbuffer.h"
#include "dx12_depthbuffer.h"

DXState gDX;

HostLookupTable<DXColorBufferData, GX2ColorBuffer> colorBufferLkp;
HostLookupTable<DXDepthBufferData, GX2DepthBuffer> depthBufferLkp;

DXColorBufferData * dx::getColorBuffer(GX2ColorBuffer *buffer) {
   return colorBufferLkp.get(buffer);
}

DXDepthBufferData * dx::getDepthBuffer(GX2DepthBuffer *buffer) {
   return depthBufferLkp.get(buffer);
}

void dx::initialise()
{
   gDX.viewport.Width = static_cast<float>(platform::ui::width());
   gDX.viewport.Height = static_cast<float>(platform::ui::height());
   gDX.viewport.MaxDepth = 1.0f;

   gDX.scissorRect.right = static_cast<LONG>(platform::ui::width());
   gDX.scissorRect.bottom = static_cast<LONG>(platform::ui::height());


#ifdef _DEBUG
   // Enable the D3D12 debug layer.
   {
      ComPtr<ID3D12Debug> debugController;
      if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
      {
         debugController->EnableDebugLayer();
      }
   }
#endif

   ComPtr<IDXGIFactory4> factory;
   ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

   // Always use WARP for now...
   static const bool USE_WARP_DEVICE = true;
   if (USE_WARP_DEVICE)
   {
      ComPtr<IDXGIAdapter> warpAdapter;
      ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

      ThrowIfFailed(D3D12CreateDevice(
         warpAdapter.Get(),
         D3D_FEATURE_LEVEL_11_0,
         IID_PPV_ARGS(&gDX.device)
         ));
   } else
   {
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
   swapChainDesc.BufferDesc.Width = platform::ui::width();
   swapChainDesc.BufferDesc.Height = platform::ui::height();
   swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   swapChainDesc.OutputWindow = (HWND)platform::ui::hwnd();
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
      CD3DX12_DESCRIPTOR_RANGE ranges[1];
      ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

      CD3DX12_ROOT_PARAMETER rootParameters[1];
      rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

      D3D12_STATIC_SAMPLER_DESC sampler = {};
      sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.MipLODBias = 0;
      sampler.MaxAnisotropy = 0;
      sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
      sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      sampler.MinLOD = 0.0f;
      sampler.MaxLOD = D3D12_FLOAT32_MAX;
      sampler.ShaderRegister = 0;
      sampler.RegisterSpace = 0;
      sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

      CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
      rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

      ComPtr<ID3DBlob> signature;
      ComPtr<ID3DBlob> error;
      ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
      ThrowIfFailed(gDX.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&gDX.rootSignature)));
   }

   // Create the pipeline state, which includes compiling and loading shaders.
   {
      ComPtr<ID3DBlob> vertexShader;
      ComPtr<ID3DBlob> pixelShader;

#ifdef _DEBUG
      // Enable better shader debugging with the graphics debugging tools.
      UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
      UINT compileFlags = 0;
#endif

      ThrowIfFailed(D3DCompileFromFile(L"resources/shaders/screendraw.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
      ThrowIfFailed(D3DCompileFromFile(L"resources/shaders/screendraw.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

      // Define the vertex input layout.
      D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
      {
         { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
         { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
      };

      {
         // Describe and create the graphics pipeline state object (PSO).
         D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
         psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
         psoDesc.pRootSignature = gDX.rootSignature.Get();
         psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
         psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
         psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
         psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
         psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
         psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
         psoDesc.DepthStencilState.DepthEnable = FALSE;
         psoDesc.DepthStencilState.StencilEnable = FALSE;
         psoDesc.SampleMask = UINT_MAX;
         psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
         psoDesc.NumRenderTargets = 1;
         psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
         psoDesc.SampleDesc.Count = 1;
         ThrowIfFailed(gDX.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&gDX.pipelineState)));
      }

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

   // Create the command list.
   gDX.frameIndex = gDX.swapChain->GetCurrentBackBufferIndex();

   ThrowIfFailed(gDX.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gDX.commandAllocator[gDX.frameIndex].Get(), gDX.emuPipelineState.Get(), IID_PPV_ARGS(&gDX.commandList)));



   {
#define SCREENSPACE(x, y) { -1 + ((x) / (float)platform::ui::width()) * 2, 1 - ((y) / (float)platform::ui::height()) * 2, 0.0f }
      float tvX = 0;
      float tvY = 0;
      float tvWidth = (float)platform::ui::tvWidth();
      float tvHeight = (float)platform::ui::tvHeight();
      float drcX = ((float)platform::ui::tvWidth() - (float)platform::ui::drcWidth()) / 2;
      float drcY = (float)platform::ui::tvHeight();
      float drcWidth = (float)platform::ui::drcWidth();
      float drcHeight = (float)platform::ui::drcHeight();

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
      ThrowIfFailed(gDX.vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pVertexDataBegin)));
      memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
      gDX.vertexBuffer->Unmap(0, nullptr);

      // Initialize the vertex buffer view.
      gDX.vertexBufferView.BufferLocation = gDX.vertexBuffer->GetGPUVirtualAddress();
      gDX.vertexBufferView.StrideInBytes = sizeof(Vertex);
      gDX.vertexBufferView.SizeInBytes = vertexBufferSize;
   }

   // 10MB Temporary Vertex Buffer
   gDX.ppcVertexBuffer = new DXDynBuffer(gDX.device.Get(), 10 * 1024 * 1024);

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

   // Command list allocators can only be reset when the associated 
   // command lists have finished execution on the GPU; apps should use 
   // fences to determine GPU execution progress.
   ThrowIfFailed(gDX.commandAllocator[gDX.frameIndex]->Reset());

   // However, when ExecuteCommandList() is called on a particular command 
   // list, that command list can then be reset at any time and must be before 
   // re-recording.
   ThrowIfFailed(gDX.commandList->Reset(gDX.commandAllocator[gDX.frameIndex].Get(), gDX.pipelineState.Get()));

   // Set necessary state.
   gDX.commandList->SetGraphicsRootSignature(gDX.rootSignature.Get());

   ID3D12DescriptorHeap* ppHeaps[] = { *gDX.srvHeap };
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

   // Render TV ScanBuffer
   {
      gDX.commandList->SetGraphicsRootDescriptorTable(0, *gDX.tvScanBuffer->srv);
      gDX.commandList->DrawInstanced(4, 1, 0, 0);
   }

   // Render DRC ScanBuffer
   {
      gDX.commandList->SetGraphicsRootDescriptorTable(0, *gDX.drcScanBuffer->srv);
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

void dx::updateBuffers()
{
   D3D12_VERTEX_BUFFER_VIEW buffers[32];

   for (auto i = 0; i < 32; ++i) {
      auto& buffer = gDX.state.attribBuffers[0];
      if (!buffer.buffer || !buffer.size) {
         buffers[i].BufferLocation = NULL;
         buffers[i].SizeInBytes = 0;
         buffers[i].StrideInBytes = 0;
         continue;
      }

      buffers[i] = *(D3D12_VERTEX_BUFFER_VIEW*)gDX.ppcVertexBuffer->get(
         buffer.stride, buffer.size, buffer.buffer);
   }

   gDX.commandList->IASetVertexBuffers(0, 32, buffers);
}

#endif
