#include "D3D12Context.h"

#include <d3dcompiler.h>

#include "D3D12Header.h"   
#include "Shell.h"
#include "Engine.h"
#include "Utils.h"

// We won't want to include these but we're doing it for now so we can build enough functionality to be able to restructure it when we a) have enough idea of the functionality we want and b) would actually benefit from doing so.
#include "Camera.h"
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>


// Defines /////////////////////////////////////////////////////////////////////////////////

#define TEAPOT_FILE "../Data/Models/teapot.obj"
#define USE_HARDCODED_SCENE 0

// Local Functions  ////////////////////////////////////////////////////////////////////////

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

// Member Functions  ///////////////////////////////////////////////////////////////////////

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
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        m_device->CreateTexture2D(heapProps, WINDOW_WIDTH, WINDOW_HEIGHT, 1, DXGI_FORMAT_D32_FLOAT, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &m_depthStencil);

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

    m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, &m_cmdAllocator);
    
    m_device->CreateFence(0, &m_fence);
}

void D3D12Context::CreateDefaultRootSignature()
{
    D3D12_ROOT_SIGNATURE_DESC desc = {};

    D3D12_ROOT_PARAMETER params[2];
    D3D12_ROOT_PARAMETER& matViewParam = params[0];
    matViewParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    matViewParam.Constants.Num32BitValues = sizeof(Matrix4x4) / 4;
    matViewParam.Constants.RegisterSpace = 0;
    matViewParam.Constants.ShaderRegister = 0;
    matViewParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_DESCRIPTOR_TABLE table = {};

    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;
    range.OffsetInDescriptorsFromTableStart = 0;

    table.NumDescriptorRanges = 1;
    table.pDescriptorRanges = &range;

    D3D12_ROOT_PARAMETER& textureTableParam = params[1];
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
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    desc.NumParameters = 2;
    desc.pParameters = params;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &samplerDesc;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error));

    m_device->CreateRootSignature(signature.Get(), &m_defaultRootSignature);
}

static void sLoadTeapot(D3D12Context& context)
{
    Assimp::Importer importer;

    const aiScene* teapotScene = importer.ReadFile(TEAPOT_FILE, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder );
    aiMesh* teapot = teapotScene->mMeshes[0];
    assert(teapot);
    assert(teapot->HasFaces());
    assert(teapot->HasPositions());

    //Create Vertex Buffer
    Vertex* verts = nullptr;
    if (teapot->HasVertexColors(0))
    {
        verts = (Vertex*)teapot->mVertices;
    }
    else
    {
        verts = new Vertex[teapot->mNumVertices];
        for (uint32 i = 0; i < teapot->mNumVertices; i++)
        {
            verts[i].col[0] = 1.0f;
            verts[i].col[1] = 1.0f;
            verts[i].col[2] = 1.0f;
            verts[i].col[3] = 1.0f;

            verts[i].pos[0] = teapot->mVertices[i].x;
            verts[i].pos[1] = teapot->mVertices[i].y;
            verts[i].pos[2] = teapot->mVertices[i].z;

            verts[i].normal[0] = teapot->mNormals[i].x;
            verts[i].normal[1] = teapot->mNormals[i].y;
            verts[i].normal[2] = teapot->mNormals[i].z;
        }

    }
    context.CreateVertexBuffer(teapot->mNumVertices, verts);

    // Create Index Buffer
    std::vector<uint32> indices;
    indices.reserve(3 * teapot->mNumFaces);
    for (uint32 dwFace = 0; dwFace < teapot->mNumFaces; dwFace++)
    {
        const aiFace& face = teapot->mFaces[dwFace];
        for (uint32 indexIndex = 0; indexIndex < face.mNumIndices; indexIndex++)
        {
            indices.push_back(face.mIndices[indexIndex]);
        }
    }
    context.CreateIndexBuffer(indices.size(), indices.data());
}

static void sLoadCube(D3D12Context& context)
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
    context.CreateVertexBuffer(_countof(verts), verts);

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
    context.CreateIndexBuffer(_countof(indexData), indexData);
}

void D3D12Context::LoadInitialAssets()
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

    sLoadTeapot(*this);

    // Create dumb texture
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = 1;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        m_device->CreateDescriptorHeap(desc, &m_srvDescriptorHeap);

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        int32 data = 0xFFFFFFFF;
        CreateTexture2D(heapProps, 1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &data, &m_texture);

        D3D12_CPU_DESCRIPTOR_HANDLE handle = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_device->CreateShaderResourceView(m_texture.Get(), NULL, handle);
    }

    // For buffer upload
    ExecuteCommandList();
    WaitForGPU();
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

    m_cmdList->SetGraphicsRootSignature(m_defaultRootSignature.Get());

    float radius = 10.0f;
    float time = GetCurrentFrameTime();

    // This is terrible, but fine for now
    Vector3 eyePos(radius * cosf(time), 0.0f, radius * sinf(time));
    Vector3 targetPos;
    Vector3 camUp(0.0f, 1.0f, 0.0f);
    camUp.Normalize();
    Camera cam(eyePos, targetPos, camUp, 0.1f, 100.0f, 90.0f, GetWindowAspectRatio());
    Matrix4x4 matMVP;
    cam.GetViewProjMatrix(matMVP);
    Matrix4x4 matMVPTranspose;
    matMVP.Transpose(matMVPTranspose);

    m_cmdList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix4x4) / 4, &matMVPTranspose, 0);
    
    ID3D12DescriptorHeap* heaps[] = { m_srvDescriptorHeap.Get() };
    m_cmdList->SetDescriptorHeaps(1, heaps);
    m_cmdList->SetGraphicsRootDescriptorTable(1, m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

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

    // Draw
    const float clearColor[] = { 86.0f / 255.0f, 0.0f / 255.0f, 94.0f / 255.0f, 1.0f };
    m_cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_cmdList->ClearDepthStencilView(dsvHadle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->IASetVertexBuffers(0, 1, &m_vertexBuffers[0].view);
    m_cmdList->IASetIndexBuffer(&m_indexBuffers[0].view);
    m_cmdList->DrawIndexedInstanced((uint32)m_indexBuffers[0].indexCount, 1, 0, 0, 0);

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


void D3D12Context::CreateTexture2D(
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
    m_device->CreateTexture2D(heapProps, width, height, mipLevels, format, heapFlags, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, ppTexture);

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
}

VertexBufferID D3D12Context::CreateVertexBuffer(
    size_t vertexCount,
    Vertex* vertexData)
{
    m_vertexBuffers.emplace_back();
    VertexBuffer& vertexBuffer = m_vertexBuffers.back();

    size_t vertsTotalSize = sizeof(Vertex) * vertexCount;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    CreateBuffer(heapProps, (uint32)vertsTotalSize, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, (void*)vertexData, &vertexBuffer.buffer);

    vertexBuffer.view.BufferLocation = vertexBuffer.buffer->GetGPUVirtualAddress();
    vertexBuffer.view.StrideInBytes = sizeof(Vertex);
    vertexBuffer.view.SizeInBytes = (uint32)vertsTotalSize;

    vertexBuffer.vertexCount = vertexCount;
    return (VertexBufferID)(m_vertexBuffers.size() - 1);
}

IndexBufferID D3D12Context::CreateIndexBuffer(
    size_t indexCount,
    uint32* indexData)
{
    m_indexBuffers.emplace_back();
    IndexBuffer& indexBuffer = m_indexBuffers.back();

    size_t indexTotalSize = sizeof(uint32) * indexCount;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    CreateBuffer(heapProps, (uint32)indexTotalSize, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, (void*)indexData, &indexBuffer.buffer);

    indexBuffer.view.BufferLocation = indexBuffer.buffer->GetGPUVirtualAddress();
    indexBuffer.view.SizeInBytes = sizeof(uint32) * (uint32)indexCount;
    indexBuffer.view.Format = DXGI_FORMAT_R32_UINT;

    indexBuffer.indexCount = indexCount;
    return (IndexBufferID)(m_indexBuffers.size() - 1);
}
