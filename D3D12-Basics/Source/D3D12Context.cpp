#include "D3D12Context.h"

#include "d3dcompiler.h"
#include "d3dx12.h"

#include "Shell.h"
#include "Engine.h"

// We won't want to include camera down the line, but set matrices in constant buffers via some interface which doesn't 
#include "Camera.h"

#define ASSERT_SUCCEEDED(x) {HRESULT result = x; /*assert(SUCCEEDED(result));*/} 


D3D12Context::D3D12Context()
{
    if (tGlobals.fD3DDebug)
    {
        // Enable CPU-level validation
        ID3D12Debug1* debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            debugController->SetEnableGPUBasedValidation(tGlobals.fGPUValidation);
            debugController->Release();
        }
    }

    m_viewport = {};
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.Width = (float)GetWindowWidth();
    m_viewport.Height = (float)GetWindowHeight();

    m_scissorRect = {};
    m_scissorRect.left = 0;
    m_scissorRect.right = GetWindowWidth();
    m_scissorRect.top = 0;
    m_scissorRect.bottom = GetWindowHeight();


    InitialisePipeline();

    LoadInitialAssets();
}

static void sCompileShader(const char* entryPoint, bool fIsVertexShader, ID3DBlob** shaderBlob)
{
    ComPtr<ID3DBlob> error;

    UINT compileFlags = 0;
    if (tGlobals.fD3DDebug)
    {
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    }

    HRESULT result = D3DCompileFromFile(
        SHADER_FILE,
        nullptr,
        nullptr,
        entryPoint,
        fIsVertexShader ? "vs_5_0" : "ps_5_0",
        compileFlags,
        0,
        shaderBlob,
        &error);

    if (!SUCCEEDED(result))
    {
        if (error)
        {
            EngineLog((char*)error->GetBufferPointer());
        }
    }
}

void D3D12Context::InitialisePipeline()
{
    // Create Device
    {
        ASSERT_SUCCEEDED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));
    }

    // Create Command Queue
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

        ASSERT_SUCCEEDED(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_cmdQueue)));
    }

    // Create Swapchain
    {
        HRESULT result = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory2));
        ASSERT(SUCCEEDED(result));

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = GetWindowWidth();
        desc.Height = GetWindowHeight();
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = NUM_SWAP_CHAIN_BUFFERS;
        desc.Stereo = false;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Scaling = DXGI_SCALING_NONE;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        ComPtr<IDXGISwapChain1> m_SwapChain31;
        ASSERT_SUCCEEDED(m_dxgiFactory2->CreateSwapChainForHwnd(
            m_cmdQueue.Get(),
            GetNativeViewHandle(),
            &desc,
            NULL,
            NULL,
            &m_SwapChain31));

        m_SwapChain31.As(&m_SwapChain3);

        m_frameIndex = m_SwapChain3->GetCurrentBackBufferIndex();
    }

    // Create RTV Descriptor Heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = NUM_SWAP_CHAIN_BUFFERS;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ASSERT_SUCCEEDED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));
    }

    // Create RTV Descriptors
    {
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_rtvDescriptorSize);

        for (int32 i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++)
        {
            ASSERT_SUCCEEDED(m_SwapChain3->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), NULL, handle);
            handle.Offset(1, m_rtvDescriptorSize);
        }
    }

    // Create Command Allocator
    {
        ASSERT_SUCCEEDED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator)));
    }
}

void D3D12Context::CreateBuffer(
    const D3D12_HEAP_PROPERTIES& heapProps,
    uint32 size,
    ID3D12Resource** ppBuffer)
{
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = size;
    // The following are required for all buffers
    resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // 64KB
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ASSERT_SUCCEEDED(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, // Required starting state of upload buffer
        nullptr,
        IID_PPV_ARGS(ppBuffer)
    ));
}

void D3D12Context::CreateBuffer(
    const D3D12_HEAP_PROPERTIES& heapProps,
    uint32 size,
    ID3D12Resource** ppBuffer,
    void* initialData)
{
    CreateBuffer(heapProps, size, ppBuffer);

    // Copy data into buffer
    void* pBufferData;

    D3D12_RANGE range = { 0, 0 };
    ASSERT_SUCCEEDED((*ppBuffer)->Map(0, &range, (void**)&pBufferData));
    memcpy(pBufferData, initialData, size);
    (*ppBuffer)->Unmap(0, nullptr);
}

void D3D12Context::LoadInitialAssets()
{
    // Create empty root signature for shaders with no bound resources
    {
        D3D12_ROOT_SIGNATURE_DESC desc;
        
        D3D12_ROOT_PARAMETER rootParam;
        rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParam.Constants.Num32BitValues = sizeof(Matrix4x4) / 4;
        rootParam.Constants.RegisterSpace = 0;
        rootParam.Constants.ShaderRegister = 0;
        rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; 

        desc.NumParameters = 1;
        desc.pParameters = &rootParam;
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error));

        m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_emptyRootSignature));
    }

    // Compile Shaders and create PSO
    ComPtr<ID3DBlob> vertexShader;
    sCompileShader("HelloTriangleVS", true, &vertexShader);

    ComPtr<ID3DBlob> pixelShader;
    sCompileShader("HelloTrianglePS", false, &pixelShader);

    // Create PSO
    {
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_emptyRootSignature.Get();
        desc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        desc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.DepthStencilState.DepthEnable = false;
        desc.DepthStencilState.StencilEnable = false;
        desc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;

        ASSERT_SUCCEEDED(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create Command List
    {
        ASSERT_SUCCEEDED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_cmdList)));
        m_cmdList->Close();
    }

    // Create Vertex Buffer
    {
        struct Vertex
        {
            float pos[3];
            float col[4];
        };

        const float aspectRatio = GetWindowAspectRatio();


        Vertex vertsReal[] = {
            { { 1, 1, 1 }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { 1, 1, -1 }, { 1.0f, 1.0f, 0.0f, 1.0f } },
            { { 1, -1, 1 }, { 1.0f, 0.0f, 1.0f, 1.0f } },
            { { 1, -1, -1 }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { -1, 1, 1 }, { 0.0f, 1.0f, 1.0f, 1.0f } },
            { { -1, 1, -1 }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -1, -1, 1 }, { 0.0f, 0.0f, 1.0f, 1.0f } },
            { { -1, -1, -1 }, { 0.0f, 0.0f, 0.0f, 1.0f } },
        };

        // It's bad to use an upload buffer for verts, but use one here for simplicity (more than one read?)
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        CreateBuffer(heapProps, sizeof(vertsReal), &m_vertexBuffer, (void*)vertsReal);

        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = sizeof(vertsReal);
    }
    
    // Create Index Buffer
    {
        uint16 indices[] = {
        // Front Face
            0, 1, 4,
            4, 1, 5,
        // Right Face
            2, 3, 1,
            2, 1, 0,
        // Back Face
            2, 7, 3,
            6, 7, 2,
        // Left Face
            4, 5, 7,
            6, 4, 7,
        // Top Face
            0, 4, 2,
            4, 6, 2,
        // Bottom Face
            1, 3, 7,
            7, 5 ,1,
        };

        // Again, use an upload buffer for simplicity but probably not the best
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        DXGI_FORMAT eFormat = DXGI_FORMAT_R16_UINT;

        uint32 dwSize = sizeof(indices);

        CreateBuffer(heapProps, dwSize, &m_indexBuffer, (void*)indices);

        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = dwSize;
        m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }

    // Create Fence and wait for upload
    {
        ASSERT_SUCCEEDED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

        const UINT64 fence = m_fenceValue++;
        ASSERT_SUCCEEDED(m_cmdQueue->Signal(m_fence.Get(), fence));

        if (m_fence->GetCompletedValue() < fence)
        {
            EngineLog("Waiting for Vertex Buffer Upload");
            ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(fence, m_fenceEvent))
                WaitForSingleObject(m_fenceEvent, INFINITE);
            EngineLog("Upload Finished!");
        }
    }
}

void D3D12Context::Draw()
{
    ASSERT_SUCCEEDED(m_cmdAllocator->Reset());
    ASSERT_SUCCEEDED(m_cmdList->Reset(m_cmdAllocator.Get(), m_pipelineState.Get()));

    m_cmdList->SetGraphicsRootSignature(m_emptyRootSignature.Get());

    float radius = 10.0f;
    float time = GetCurrentFrameTime();

    Vector3 vecEye(radius * cosf(time), radius * sinf(time), 0.0f);
    Vector3 vecTarget;
    Vector3 vecUp(0.0f, 0.0f, 1.0f);
    Camera cam(vecEye, vecTarget, vecUp, 0.1f, 100.0f, 90.0f, GetWindowAspectRatio());
    Matrix4x4 matMVP;
    cam.GetViewProjMatrix(matMVP);
    Matrix4x4 matMVPTranspose;
    matMVP.Transpose(matMVPTranspose);

    m_cmdList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix4x4) / 4, &matMVPTranspose, 0);

    m_cmdList->RSSetViewports(1, &m_viewport);
    m_cmdList->RSSetScissorRects(1, &m_scissorRect);

    // Transition back buffer to rt
    {
        D3D12_RESOURCE_TRANSITION_BARRIER transition;
        transition.pResource = m_renderTargets[m_frameIndex].Get();
        transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        transition.Subresource = 0;

        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition = transition;

        m_cmdList->ResourceBarrier(1, &barrier);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    rtvHandle.ptr = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + (UINT64)m_frameIndex * (UINT64)m_rtvDescriptorSize;

    m_cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

    // Draw
    const float clearColor[] = { 86.0f / 255.0f, 0.0f / 255.0f, 94.0f / 255.0f, 1.0f };
    m_cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_cmdList->IASetIndexBuffer(&m_indexBufferView);
    m_cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);


    // Transition back buffer to present
    {
        D3D12_RESOURCE_TRANSITION_BARRIER transition;
        transition.pResource = m_renderTargets[m_frameIndex].Get();
        transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        transition.Subresource = 0;

        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition = transition;

        m_cmdList->ResourceBarrier(1, &barrier);
    }

    ASSERT_SUCCEEDED(m_cmdList->Close());

    ID3D12CommandList* ppCmdLists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(_countof(ppCmdLists), ppCmdLists);
}

void D3D12Context::Present()
{
    ASSERT_SUCCEEDED(m_SwapChain3->Present(1, 0));

    const UINT64 fence = m_fenceValue++;
    ASSERT_SUCCEEDED(m_cmdQueue->Signal(m_fence.Get(), fence));

    // Ideally, we'd just continue to prepare the next frame, but for simplicity just wait for it for now.
    if (m_fence->GetCompletedValue() < fence)
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(fence, m_fenceEvent))
            WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_SwapChain3->GetCurrentBackBufferIndex();
}