#pragma once

#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

class Device
{
public:

    void CreateCommandQueue(
        const D3D12_COMMAND_QUEUE_DESC& desc,
        ID3D12CommandQueue** ppCmdQueue);

    void CreateGraphicsCommandList(
        ID3D12CommandAllocator* cmdAllocator,
        ID3D12PipelineState* pipelineState,
        ID3D12GraphicsCommandList** ppCmdList);

    void CreateCommandAllocator(
        const D3D12_COMMAND_LIST_TYPE& desc,
        ID3D12CommandAllocator** ppCmdQueue);

    void CreateDescriptorHeap(
        const D3D12_DESCRIPTOR_HEAP_DESC& desc,
        ID3D12DescriptorHeap** ppDescriptorHeap,
        uint32& descriptorSizeOut);

    void CreateRenderTargetView(
        ID3D12Resource* resource,
        D3D12_RENDER_TARGET_VIEW_DESC* desc,
        D3D12_CPU_DESCRIPTOR_HANDLE& rtvOut);


    void CreateShaderResourceView(
        ID3D12Resource* resource,
        D3D12_SHADER_RESOURCE_VIEW_DESC* desc,
        D3D12_CPU_DESCRIPTOR_HANDLE& rtvOut);

    void CreateRootSignature(
        ID3DBlob* signatureBlob,
        ID3D12RootSignature** rootSignature);

    void CreateGraphicsPipelineState(
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
        ID3D12PipelineState** ppPipelineState);

    void CreateFence(
        UINT64 initialValue,
        ID3D12Fence** fence);

    D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(
        uint32 numResourceDescs,
        const D3D12_RESOURCE_DESC* resourceDescs);

    void GetCopyableFootprints(
        const D3D12_RESOURCE_DESC* desc,
        UINT firstSubresource,
        UINT numSubresources,
        UINT64 baseOffset,
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts,
        UINT* numRows,
        UINT64* rowSizeInBytes,
        UINT64* totalBytes);

    void CreateBuffer(
        const D3D12_HEAP_PROPERTIES& heapProps,
        size_t size,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_STATES initialState,
        ID3D12Resource** ppBuffer);

    void CreateTexture2D(
        const D3D12_HEAP_PROPERTIES& heapProps,
        const D3D12_RESOURCE_DESC& desc,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_STATES initialState,
        ID3D12Resource** ppTexture);

    Device();

private:
    ComPtr<ID3D12Device> m_device;

#ifdef _DEBUG
    uint32 m_debugResourceIndex = 0;
#endif
};

