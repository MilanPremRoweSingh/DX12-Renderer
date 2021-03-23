#pragma once

#include <dxgi1_6.h>
#include <vector>

#include "Renderer.h"
#include "D3D12Header.h"
#include "Device.h"
#include "UploadStream.h"

// Enums ///////////////////////////////////////////////////////////////////////////////////

enum VertexBufferID : int32
{
    VertexBufferIDInvalid = -1
};

enum IndexBufferID : int32
{
    IndexBufferIDInvalid = -1
};

// Structs /////////////////////////////////////////////////////////////////////////////////

struct IndexBuffer
{
    ComPtr<ID3D12Resource> buffer;
    D3D12_INDEX_BUFFER_VIEW view;
    size_t indexCount;
};

struct VertexBuffer
{
    ComPtr<ID3D12Resource> buffer;
    D3D12_VERTEX_BUFFER_VIEW view;
    size_t vertexCount;
};

struct ConstantBuffer
{
    ComPtr<ID3D12Resource> buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE cbv;
    size_t size;
};

// Support a single vertex format for now
struct Vertex
{
    float pos[3];
    float col[4];
    float normal[4];
};

// Classes /////////////////////////////////////////////////////////////////////////////////

class D3D12Context
{
public:
    D3D12Context();
    ~D3D12Context();

    void InitialisePipeline(
        void);

    void LoadInitialAssets(
        void);

    void ExecuteCommandList(
        void);

    void WaitForGPU(
        void);

    void Draw(
        void);

    void Present(
        void);

    void CreateBuffer(
        const D3D12_HEAP_PROPERTIES& heapProps,
        uint32 size,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_STATES initialState,
        void* initialData,
        ID3D12Resource** ppBuffer);

    void CreateTexture2D(
        const D3D12_HEAP_PROPERTIES& heapProps,
        uint32 width,
        uint32 height,
        uint16 mipLevels,
        DXGI_FORMAT format,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_STATES initialState,
        void* initialData,
        ID3D12Resource** ppTexture);

    VertexBufferID CreateVertexBuffer(
        size_t vertexCount,
        Vertex* vertexData);

    IndexBufferID CreateIndexBuffer(
        size_t indexCount,
        uint32* indexData);

    void CreateConstantBuffer(
        size_t size);

 private:

     void CreateDefaultRootSignature(
         void);

     D3D12_VIEWPORT m_viewport;
     D3D12_RECT  m_scissorRect;

     Device* m_device;
     ComPtr<ID3D12CommandQueue> m_cmdQueue;
     ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
     ComPtr<ID3D12GraphicsCommandList> m_cmdList;

     ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
     ComPtr<ID3D12Resource> m_renderTargets[NUM_SWAP_CHAIN_BUFFERS];
     uint32 m_rtvDescriptorSize;

     ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
     ComPtr<ID3D12Resource> m_depthStencil;

     ComPtr<ID3D12RootSignature> m_defaultRootSignature;

     ComPtr<ID3D12DescriptorHeap> m_cbvDescriptorHeap;
     ConstantBuffer m_constantBuffer;

     // 'General' i.e. CBV + SRV + UAV
     ComPtr<ID3D12DescriptorHeap> m_generalDescriptorHeap;
     uint32 m_generalDescriptorSize;
     CD3DX12_CPU_DESCRIPTOR_HANDLE m_nextGeneralDescriptor;

     ComPtr<ID3D12Resource> m_texture;
     
     ComPtr<ID3D12PipelineState> m_pipelineState;

     UploadStream* m_uploadStream;
     std::vector<VertexBuffer> m_vertexBuffers;
     std::vector<IndexBuffer> m_indexBuffers;

     uint32 m_frameIndex = 0;
     HANDLE m_fenceEvent;
     ComPtr<ID3D12Fence> m_fence;
     UINT64 m_fenceValue;

     // DXGI
     ComPtr<IDXGIFactory2> m_dxgiFactory2;
     ComPtr<IDXGISwapChain3> m_swapChain3;
};