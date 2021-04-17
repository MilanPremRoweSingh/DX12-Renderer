#include "D3D12Core.h"

#include <d3dcompiler.h>

#include "D3D12Header.h"   
#include "Shell.h"
#include "Engine.h"

// We won't want to include these but we're doing it for now so we can build enough functionality to be able to restructure it when we a) have enough idea of the functionality we want and b) would actually benefit from doing so.
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>


// Defines /////////////////////////////////////////////////////////////////////////////////

#define USE_HARDCODED_SCENE 0
#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)

// Local Functions  ////////////////////////////////////////////////////////////////////////

static void sCompileShader(
    const char* entryPoint, 
    bool fIsVertexShader, 
    ID3DBlob** shaderBlob)
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
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
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

// Member Functions  ///////////////////////////////////////////////////////////////////////

D3D12Core::D3D12Core()
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
    InitialAssetsLoad();
}

D3D12Core::~D3D12Core()
{
    delete m_uploadStream;
    delete m_device;
}

void D3D12Core::InitialisePipeline()
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

    // Create RTV Descriptor Heap and RTV Descriptors
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = NUM_SWAP_CHAIN_BUFFERS;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        m_device->CreateDescriptorHeap(desc, &m_rtvDescriptorHeap, m_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_rtvDescriptorSize);

        for (int32 i = 0; i < NUM_SWAP_CHAIN_BUFFERS; i++)
        {
            ASSERT_SUCCEEDED(m_swapChain3->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), NULL, handle);
            handle.Offset(1, m_rtvDescriptorSize);
        }
    }

    // Create DSV Descriptor Heap and DSV Descriptor
    {        
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        m_device->CreateTexture2D(heapProps, WINDOW_WIDTH, WINDOW_HEIGHT, 1, DXGI_FORMAT_D32_FLOAT, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValue, &m_depthStencil);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
        descHeapDesc.NumDescriptors = 1;
        descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        m_device->CreateDescriptorHeap(descHeapDesc, &m_dsvDescriptorHeap);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, handle);
    }

    // Create General descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = 2;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        m_device->CreateDescriptorHeap(desc, &m_generalDescriptorHeap, m_generalDescriptorSize);
        m_nextGeneralDescriptor= { m_generalDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),  m_generalDescriptorHeap->GetGPUDescriptorHandleForHeapStart() };
    }

    ConstantBuffersInit();

    m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, &m_cmdAllocator);
    
    m_device->CreateFence(0, &m_fence);
}

void D3D12Core::CreateDefaultRootSignature()
{
    D3D12_ROOT_SIGNATURE_DESC desc = {};

    D3D12_ROOT_PARAMETER params[CBCount + 1];
    uint32 idxParam = 0;

    for (int32 id = 0; id < CBCount; id++)
    {
        D3D12_ROOT_PARAMETER& cbvParam = params[idxParam++];
        cbvParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        cbvParam.Descriptor.RegisterSpace = 0;
        cbvParam.Descriptor.ShaderRegister = id;
        cbvParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    D3D12_ROOT_DESCRIPTOR_TABLE table = {};

    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;
    range.OffsetInDescriptorsFromTableStart = 0;

    table.NumDescriptorRanges = 1;
    table.pDescriptorRanges = &range;

    D3D12_ROOT_PARAMETER& textureTableParam = params[idxParam++];
    textureTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    textureTableParam.DescriptorTable = table;
    textureTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = 0;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    desc.NumParameters = idxParam;
    desc.pParameters = params;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &samplerDesc;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error));

    m_device->CreateRootSignature(signature.Get(), &m_defaultRootSignature);
}

void D3D12Core::ConstantBuffersInit(
    void)
{
    for (int32 id = CBStart; id < CBCount; id++)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        m_device->CreateBuffer(heapProps, Utils::AlignUp(g_cbSizes[id], (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, &m_constantBuffers[id]);
    }
}

void D3D12Core::ConstantBufferSetData(
    ConstantBufferID id,
    size_t size,
    void* data)
{
    D3D12_RANGE range = { 0,0 };
    void* cbData;
    m_constantBuffers[id]->Map(0, &range, &cbData);
    memcpy(cbData, data, g_cbSizes[id]);
    range = { 0, g_cbSizes[id] };
    m_constantBuffers[id]->Unmap(0, &range);
}

static void sLoadCube(D3D12Core& context)
{
    // With Normals now, we need to dupe verts for it work properly if we want proper shading
    Vertex verts[] = {
        { { 1, 1, 1 }, { 1.0f, 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f, 1.0f} },
        { { 1, 1, -1 }, { 1.0f, 1.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f} },
        { { 1, -1, 1 }, { 1.0f, 0.0f, 1.0f, 1.0f }, {1.0f, 1.0f, 1.0f} },
        { { 1, -1, -1 }, { 1.0f, 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f} },
        { { -1, 1, 1 }, { 0.0f, 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f, 1.0f} },
        { { -1, 1, -1 }, { 0.0f, 1.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f} },
        { { -1, -1, 1 }, { 0.0f, 0.0f, 1.0f, 1.0f }, {1.0f, 1.0f, 1.0f} },
        { { -1, -1, -1 }, { 0.0f, 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f} },
    };
    context.VertexBufferCreate(_countof(verts), verts);

    uint32 indexData[] = {
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
    context.IndexBufferCreate(_countof(indexData), indexData);
}

void D3D12Core::InitialAssetsLoad()
{
    CreateDefaultRootSignature();

    // Compile Shaders and create PSO
    ComPtr<ID3DBlob> vertexShader;
    sCompileShader("VSMain", true, &vertexShader);

    ComPtr<ID3DBlob> pixelShader;
    sCompileShader("PSMain", false, &pixelShader);

    // Create PSO
    {
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

        inputElementDescs.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        inputElementDescs.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        inputElementDescs.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_defaultRootSignature.Get();
        desc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        desc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        desc.DepthStencilState.StencilEnable = false;
        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        desc.InputLayout = { inputElementDescs.data(), (uint32)inputElementDescs.size() };
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

    // Create dumb texture
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        int32 data = 0xFFFFFFFF;
        Texture2DCreate(heapProps, 1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &data, &m_texture);

        m_device->CreateShaderResourceView(m_texture.Get(), NULL, AllocateGeneralDescriptor().cpuHandle);
    }
}

void D3D12Core::BufferCreate(
    const D3D12_HEAP_PROPERTIES& heapProps,
    uint32 size,
    D3D12_HEAP_FLAGS heapFlags,
    D3D12_RESOURCE_STATES initialState,
    void* initialData,
    ID3D12Resource** ppBuffer)
{
    CommandListBegin();

    m_device->CreateBuffer(heapProps, size, heapFlags, D3D12_RESOURCE_STATE_COPY_DEST, ppBuffer);

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

    CommandListExecute();
}

void D3D12Core::Texture2DCreate(
    const D3D12_HEAP_PROPERTIES& heapProps,
    uint32 width,
    uint32 height,
    uint16 mipLevels,
    DXGI_FORMAT format,
    D3D12_HEAP_FLAGS heapFlags,
    D3D12_RESOURCE_STATES initialState,
    void* initialData,
    ID3D12Resource** ppTexture)
{
    CommandListBegin();

    m_device->CreateTexture2D(heapProps, width, height, mipLevels, format, heapFlags, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, nullptr, ppTexture);

    uint32 rowPitch = width * GetDXGIFormatSize(format);
    size_t slicePitch = height * rowPitch;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    footprint.Footprint.Width = width;
    footprint.Footprint.Height = height;
    footprint.Footprint.Depth = 1;
    footprint.Footprint.Format = format;
    footprint.Footprint.RowPitch = Utils::AlignUp(rowPitch, (uint32)D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    UploadStream::Allocation uploadBufferAlloc = m_uploadStream->AllocateAligned(slicePitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    footprint.Offset = uploadBufferAlloc.offset;

    uint32 alignedRowPitch = Utils::AlignUp(rowPitch, (uint32)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    for (uint32 i = 0; i < height; i++)
    {
        memcpy((INT8*)uploadBufferAlloc.cpuAddr + (i * rowPitch), initialData, width * GetDXGIFormatSize(format));
    }

    D3D12_TEXTURE_COPY_LOCATION src;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.pResource = uploadBufferAlloc.buffer;
    src.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dst;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.pResource = *ppTexture;
    dst.SubresourceIndex = 0;

    m_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    if (initialState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        D3D12_RESOURCE_TRANSITION_BARRIER transition;
        transition.pResource = *ppTexture;
        transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        transition.StateAfter = initialState;
        transition.Subresource = 0;

        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition = transition;

        m_cmdList->ResourceBarrier(1, &barrier);
    }

    CommandListExecute();
}

VertexBufferID D3D12Core::VertexBufferCreate(
    size_t vertexCount,
    Vertex* pVertexData)
{
    VertexBuffer vertexBuffer;

    size_t vertsTotalSize = sizeof(Vertex) * vertexCount;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    BufferCreate(heapProps, (uint32)vertsTotalSize, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, (void*)pVertexData, &vertexBuffer.pBuffer);

    vertexBuffer.view.BufferLocation = vertexBuffer.pBuffer->GetGPUVirtualAddress();
    vertexBuffer.view.StrideInBytes = sizeof(Vertex);
    vertexBuffer.view.SizeInBytes = (uint32)vertsTotalSize;

    vertexBuffer.vertexCount = vertexCount;

    VertexBufferID id = m_vbidAllocator.AllocID();
    m_vertexBuffers[id] = vertexBuffer;
    return id;
}

void D3D12Core::VertexBufferDestroy(
    VertexBufferID vbid)
{
    VertexBuffer& vertexBuffer = m_vertexBuffers[vbid];
    vertexBuffer.pBuffer->Release();
    m_vertexBuffers.erase(vbid);
    m_vbidAllocator.FreeID(vbid);
}

IndexBufferID D3D12Core::IndexBufferCreate(
    size_t indexCount,
    uint32* pIndexData)
{
    IndexBuffer indexBuffer;

    size_t indexTotalSize = sizeof(uint32) * indexCount;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    BufferCreate(heapProps, (uint32)indexTotalSize, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, (void*)pIndexData, &indexBuffer.pBuffer);

    indexBuffer.view.BufferLocation = indexBuffer.pBuffer->GetGPUVirtualAddress();
    indexBuffer.view.SizeInBytes = sizeof(uint32) * (uint32)indexCount;
    indexBuffer.view.Format = DXGI_FORMAT_R32_UINT;

    indexBuffer.indexCount = indexCount;


    IndexBufferID id = m_ibidAllocator.AllocID();
    m_indexBuffers[id] = indexBuffer;
    return id;
}

void D3D12Core::IndexBufferDestroy(
    IndexBufferID ibid)
{
    IndexBuffer& indexBuffer = m_indexBuffers[ibid];
    indexBuffer.pBuffer->Release();
    m_indexBuffers.erase(ibid);
    m_ibidAllocator.FreeID(ibid);
}

D3D12_DESCRIPTOR_ADDRESS D3D12Core::AllocateGeneralDescriptor(
    void)
{
    D3D12_DESCRIPTOR_ADDRESS curr = m_nextGeneralDescriptor;
    m_nextGeneralDescriptor.cpuHandle.ptr += m_generalDescriptorSize;
    m_nextGeneralDescriptor.gpuHandle.ptr += m_generalDescriptorSize;
    return curr;
}


void D3D12Core::CommandListExecute()
{
    ASSERT_SUCCEEDED(m_cmdList->Close());
    ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void D3D12Core::CommandListBegin()
{
    // BAaaaaAAaad
    WaitForGPU();
    ASSERT_SUCCEEDED(m_cmdAllocator->Reset());
    ASSERT_SUCCEEDED(m_cmdList->Reset(m_cmdAllocator.Get(), m_pipelineState.Get()));
}

void D3D12Core::WaitForGPU()
{
    const UINT64 fence = ++m_fenceValue;
    ASSERT_SUCCEEDED(m_cmdQueue->Signal(m_fence.Get(), fence));

    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void D3D12Core::Begin(
    void)
{
    CommandListBegin();

    m_cmdList->SetGraphicsRootSignature(m_defaultRootSignature.Get());

    uint32 rootParamIdx = 0;
    for (int32 id = CBStart; id < CBCount; id++)
    {
        m_cmdList->SetGraphicsRootConstantBufferView(rootParamIdx++, m_constantBuffers[id]->GetGPUVirtualAddress());
    }

    ID3D12DescriptorHeap* heaps[] = { m_generalDescriptorHeap.Get() };
    m_cmdList->SetDescriptorHeaps(rootParamIdx++, heaps);
    m_cmdList->SetGraphicsRootDescriptorTable(1, m_generalDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

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

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHadle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    m_cmdList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHadle);

    const float clearColor[] = { 86.0f / 255.0f, 0.0f / 255.0f, 94.0f / 255.0f, 1.0f };
    m_cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_cmdList->ClearDepthStencilView(dsvHadle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3D12Core::Draw(
    VertexBufferID vbid,
    IndexBufferID ibid)
{
    // Draw
    m_cmdList->IASetVertexBuffers(0, 1, &m_vertexBuffers[vbid].view);
    m_cmdList->IASetIndexBuffer(&m_indexBuffers[ibid].view);
    m_cmdList->DrawIndexedInstanced((uint32)m_indexBuffers[ibid].indexCount, 1, 0, 0, 0);
}

void D3D12Core::End()
{
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

    CommandListExecute();
}

void D3D12Core::Present()
{
    ASSERT_SUCCEEDED(m_swapChain3->Present(1, 0));

    WaitForGPU();

    m_frameIndex = m_swapChain3->GetCurrentBackBufferIndex();

    m_uploadStream->ResetAllocations();
}