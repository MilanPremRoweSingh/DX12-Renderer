#include "D3D12Context.h"

#include "D3D12Header.h"   
#include "d3dcompiler.h"

#include "Shell.h"
#include "Engine.h"

// We won't want to include camera down the line, but set matrices in constant buffers via some interface which doesn't 
#include "Camera.h"

D3D12Context::D3D12Context()
{
    if (globals.fD3DDebug)
    {
        // Enable CPU-level validation
        ID3D12Debug1* debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            debugController->SetEnableGPUBasedValidation(globals.fGPUValidation);
            debugController->Release();
        }
    }

    m_device = new Device();

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

    m_uploadStream = new UploadStream(m_device);

    InitialisePipeline();
    LoadInitialAssets();
}

D3D12Context::~D3D12Context()
{
    delete m_uploadStream;
    delete m_device;
}


static void sCompileShader(const char* entryPoint, bool fIsVertexShader, ID3DBlob** shaderBlob)
{
    ComPtr<ID3DBlob> error;

    UINT compileFlags = 0;
    if (globals.fD3DDebug)
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

    // Create Command Queue
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

        m_device->CreateCommandQueue(desc, m_cmdQueue.GetAddressOf());
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

        ComPtr<IDXGISwapChain1> swapChain;
        ASSERT_SUCCEEDED(m_dxgiFactory2->CreateSwapChainForHwnd(
            m_cmdQueue.Get(),
            GetNativeViewHandle(),
            &desc,
            NULL,
            NULL,
            &swapChain));

        swapChain.As(&m_swapChain3);

        m_frameIndex = m_swapChain3->GetCurrentBackBufferIndex();
    }

    // Create RTV Descriptor Heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = NUM_SWAP_CHAIN_BUFFERS;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        m_device->CreateDescriptorHeap(desc, &m_rtvDescriptorHeap, m_rtvDescriptorSize);
    }

    // Create RTV Descriptors
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_rtvDescriptorSize);

        for (int32 i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++)
        {
            ASSERT_SUCCEEDED(m_swapChain3->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), NULL, handle);
            handle.Offset(1, m_rtvDescriptorSize);
        }
    }

    m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, &m_cmdAllocator);
    
    m_device->CreateFence(0, &m_fence);
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

        m_device->CreateRootSignature(signature.Get(), &m_emptyRootSignature);
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

        m_device->CreateGraphicsPipelineState(desc, &m_pipelineState);
    }

    // Create Command List
    {
        m_device->CreateGraphicsCommandList(m_cmdAllocator.Get(), m_pipelineState.Get(), &m_cmdList);
    }

    // Create Vertex Buffer
    {
        struct Vertex
        {
            float pos[3];
            float col[4];
        };

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

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        CreateBuffer(heapProps, sizeof(vertsReal), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, (void*)vertsReal, &m_vertexBuffer);

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
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        uint32 size = sizeof(indices);

        CreateBuffer(heapProps, sizeof(indices), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, (void*)indices, &m_indexBuffer);

        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = size;
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    }

    // For buffer upload
    ExecuteCommandList();

    // Create Fence and wait for upload
    {
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

void D3D12Context::ExecuteCommandList()
{
    ASSERT_SUCCEEDED(m_cmdList->Close());
    ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void D3D12Context::WaitForGPU()
{
    const UINT64 fence = ++m_fenceValue;
    ASSERT_SUCCEEDED(m_cmdQueue->Signal(m_fence.Get(), fence));

    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void D3D12Context::Draw()
{
    ASSERT_SUCCEEDED(m_cmdAllocator->Reset());
    ASSERT_SUCCEEDED(m_cmdList->Reset(m_cmdAllocator.Get(), m_pipelineState.Get()));

    m_cmdList->SetGraphicsRootSignature(m_emptyRootSignature.Get());

    float radius = 10.0f;
    float time = GetCurrentFrameTime();

    // This is terrible, but fine for now
    Vector3 eyePos(radius * cosf(time), radius * sinf(time), 0.0f);
    Vector3 targetPos;
    Vector3 camUp(0.0f, 0.0f, 1.0f);
    Camera cam(eyePos, targetPos, camUp, 0.1f, 100.0f, 90.0f, GetWindowAspectRatio());
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
    
    ExecuteCommandList();
}

void D3D12Context::Present()
{
    ASSERT_SUCCEEDED(m_swapChain3->Present(1, 0));

    WaitForGPU();

    m_frameIndex = m_swapChain3->GetCurrentBackBufferIndex();

    m_uploadStream->ResetAllocations();
}


void D3D12Context::CreateBuffer(
    const D3D12_HEAP_PROPERTIES& heapProps,
    uint32 size,
    D3D12_HEAP_FLAGS heapFlags,
    D3D12_RESOURCE_STATES initialState,
    void* initialData,
    ID3D12Resource** ppBuffer)
{
    m_device->CreateBuffer(heapProps, size, heapFlags, D3D12_RESOURCE_STATE_COPY_DEST, ppBuffer);

    // Create Intermediate Upload buffer
    ComPtr<ID3D12Resource> uploadBuffer;

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    m_device->CreateBuffer(uploadHeapProps, size, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, &uploadBuffer);

    UploadStream::Allocation uploadBufferAlloc = m_uploadStream->Allocate(size);
    memcpy(uploadBufferAlloc.cpuAddr, initialData, size);    
    m_cmdList->CopyBufferRegion(*ppBuffer, 0, uploadBufferAlloc.buffer, uploadBufferAlloc.offset, size);
    if (initialState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        D3D12_RESOURCE_TRANSITION_BARRIER transition;
        transition.pResource = *ppBuffer;
        transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        transition.StateAfter = initialState;
        transition.Subresource = 0;

        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition = transition;

        m_cmdList->ResourceBarrier(1, &barrier);
    }
}