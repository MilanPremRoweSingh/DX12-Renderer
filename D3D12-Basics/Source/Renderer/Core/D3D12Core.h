#pragma once

#include <dxgi1_6.h>
#include <vector>

#include "Generic/IDAllocator.h"

#include "Renderer/VertexFormats.h"
#include "Renderer/ConstantBuffers.h"
#include "Renderer/Renderer.h"

#include "Renderer/Core/D3D12Header.h"
#include "Renderer/Core/Device.h"
#include "Renderer/Core/UploadStream.h"


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
    ID3D12Resource* pBuffer;
    D3D12_INDEX_BUFFER_VIEW view;
    size_t indexCount;
};

struct VertexBuffer
{
    ID3D12Resource* pBuffer;
    D3D12_VERTEX_BUFFER_VIEW view;
    size_t vertexCount;
};

struct ConstantBuffer
{
    ComPtr<ID3D12Resource> buffer;
    D3D12_DESCRIPTOR_ADDRESS view;
    size_t size;
};

// Classes /////////////////////////////////////////////////////////////////////////////////

class D3D12Core
{
public:
    D3D12Core();
    ~D3D12Core();

    void InitialisePipeline(
        void);

    void InitialAssetsLoad(
        void);

    void BufferCreate(
        const D3D12_HEAP_PROPERTIES& heapProps,
        uint32 size,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_STATES initialState,
        void* initialData,
        ID3D12Resource** ppBuffer);

    void Texture2DCreate(
        const D3D12_HEAP_PROPERTIES& heapProps,
        uint32 width,
        uint32 height,
        uint16 mipLevels,
        DXGI_FORMAT format,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_STATES initialState,
        void* initialData,
        ID3D12Resource** ppTexture);

    VertexBufferID VertexBufferCreate(
        size_t vertexCount,
        Vertex* pVertexData);

    void VertexBufferDestroy(
        VertexBufferID vbid);

    IndexBufferID IndexBufferCreate(
        size_t indexCount,
        uint32* pIndexData);

    void IndexBufferDestroy(
        IndexBufferID ibid);

    void ConstantBufferSetData(
        ConstantBufferID id,
        size_t size,
        void* data);

    void CommandListExecute(
        void);

    void CommandListBegin(
        void);

    void WaitForGPU(
        void);

    void Begin(
        void);

    void Draw(
        VertexBufferID vbid,
        IndexBufferID ibid);

    void End(
        void);

    void Present(
        void);


private:

    D3D12_DESCRIPTOR_ADDRESS AllocateGeneralDescriptor(
        void);

     void CreateRootSignature(
         void);

     void ConstantBuffersInit(
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

     ComPtr<ID3D12Resource> m_staticConstantBuffer;

     // We will allocate constant buffer data every time it's changed, but we only ever need a reference to the latest allocation 
     // we free the upload stream memory independent of the allocations it provides.
     UploadStream::Allocation m_dynamicConstantBufferAllocations[CBIDDynamicCount];

     // 'General' i.e. CBV + SRV + UAV
     ComPtr<ID3D12DescriptorHeap> m_generalDescriptorHeap;
     uint32 m_generalDescriptorSize;
     D3D12_DESCRIPTOR_ADDRESS m_nextGeneralDescriptor;

     ComPtr<ID3D12Resource> m_texture;
     
     ComPtr<ID3D12PipelineState> m_pipelineState;

     UploadStream* m_uploadStream;

     IDAllocator<VertexBufferID> m_vbidAllocator = IDAllocator<VertexBufferID>(VertexBufferID(0));
     std::unordered_map<VertexBufferID, VertexBuffer> m_vertexBuffers;

     IDAllocator<IndexBufferID> m_ibidAllocator = IDAllocator<IndexBufferID>(IndexBufferID(0));
     std::unordered_map<IndexBufferID, IndexBuffer> m_indexBuffers;

     uint32 m_frameIndex = 0;
     HANDLE m_fenceEvent;
     ComPtr<ID3D12Fence> m_fence;
     UINT64 m_fenceValue;

     // DXGI
     ComPtr<IDXGIFactory2> m_dxgiFactory2;
     ComPtr<IDXGISwapChain3> m_swapChain3;
};