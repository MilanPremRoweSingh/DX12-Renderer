#include "Device.h"

#include "D3D12Header.h"

#ifdef _DEBUG
#include <sstream>
#endif

Device::Device()
{
    // Create Device
    ASSERT_SUCCEEDED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));
}

void Device::CreateCommandQueue(
    const D3D12_COMMAND_QUEUE_DESC& desc,
    ID3D12CommandQueue** cmdQueue)
{
    ASSERT_SUCCEEDED(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(cmdQueue)));
}

void Device::CreateGraphicsCommandList(
    ID3D12CommandAllocator* cmdAllocator,
    ID3D12PipelineState* pipelineState,
    ID3D12GraphicsCommandList** ppCmdList)
{
    ASSERT_SUCCEEDED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, pipelineState, IID_PPV_ARGS(ppCmdList)));
}

void Device::CreateCommandAllocator(
    const D3D12_COMMAND_LIST_TYPE& cmdListType,
    ID3D12CommandAllocator** ppCmdQueue)
{
    ASSERT_SUCCEEDED(m_device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(ppCmdQueue)));
}

void Device::CreateDescriptorHeap(
    const D3D12_DESCRIPTOR_HEAP_DESC& desc,
    ID3D12DescriptorHeap** ppDescriptorHeap,
    uint32& descriptorSize)
{
    ASSERT_SUCCEEDED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(ppDescriptorHeap)));
    descriptorSize = m_device->GetDescriptorHandleIncrementSize(desc.Type);
}

void Device::CreateRenderTargetView(
    ID3D12Resource* resource,
    D3D12_RENDER_TARGET_VIEW_DESC* desc,
    D3D12_CPU_DESCRIPTOR_HANDLE& rtvOut)
{
    m_device->CreateRenderTargetView(resource, desc, rtvOut);
}

void Device::CreateShaderResourceView(
    ID3D12Resource* resource,
    D3D12_SHADER_RESOURCE_VIEW_DESC* desc,
    D3D12_CPU_DESCRIPTOR_HANDLE& rtvOut)
{
    m_device->CreateShaderResourceView(resource, desc, rtvOut);
}

void Device::CreateRootSignature(
    ID3DBlob* signatureBlob,
    ID3D12RootSignature** rootSignature)
{
    ASSERT_SUCCEEDED(m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature)));
}


void Device::CreateGraphicsPipelineState(
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
    ID3D12PipelineState** ppPipelineState)
{
    ASSERT_SUCCEEDED(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(ppPipelineState)));
}

void Device::CreateFence(
    UINT64 initialValue,
    ID3D12Fence** fence
)
{
    ASSERT_SUCCEEDED(m_device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence)));
}

void Device::CreateBuffer(
    const D3D12_HEAP_PROPERTIES& heapProps,
    size_t size,
    D3D12_HEAP_FLAGS heapFlags,
    D3D12_RESOURCE_STATES initialState,
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
        heapFlags,
        &resourceDesc,
        initialState, // Required starting state of upload buffer
        nullptr,
        IID_PPV_ARGS(ppBuffer)
    ));

#ifdef _DEBUG
    std::wstringstream ws;
    ws << "Buffer " << m_debugResourceIndex++;
    (*ppBuffer)->SetName(ws.str().c_str());
#endif
}

D3D12_RESOURCE_ALLOCATION_INFO Device::GetResourceAllocationInfo(
    uint32 numResourceDescs,
    const D3D12_RESOURCE_DESC* resourceDescs)
{
    return m_device->GetResourceAllocationInfo(0, numResourceDescs, resourceDescs);
}

void Device::GetCopyableFootprints(
    const D3D12_RESOURCE_DESC* desc,
    UINT firstSubresource,
    UINT numSubresources,
    UINT64 baseOffset,
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts,
    UINT* numRows,
    UINT64* rowSizeInBytes,
    UINT64* totalBytes)
{
    m_device->GetCopyableFootprints(
        desc,
        firstSubresource,
        numSubresources,
        baseOffset,
        layouts,
        numRows,
        rowSizeInBytes,
        totalBytes);
}

void Device::CreateTexture2D(
    const D3D12_HEAP_PROPERTIES& heapProps,
    const D3D12_RESOURCE_DESC& desc,
    D3D12_HEAP_FLAGS heapFlags,
    D3D12_RESOURCE_STATES initialState,
    ID3D12Resource** ppTexture)
{
    ASSERT_SUCCEEDED(m_device->CreateCommittedResource(
        &heapProps,
        heapFlags,
        &desc,
        initialState, // Required starting state of upload buffer
        nullptr,
        IID_PPV_ARGS(ppTexture)
    ));

#ifdef _DEBUG
    std::wstringstream ws;
    ws << "Texture " << m_debugResourceIndex++;
    (*ppTexture)->SetName(ws.str().c_str());
#endif
}
