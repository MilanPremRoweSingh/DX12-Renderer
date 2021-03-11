#pragma once

#include "Renderer.h"
#include <wrl/client.h>

#include "d3d12.h"
#include "dxgi1_6.h"

using Microsoft::WRL::ComPtr;

class D3D12Context
{
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT  m_scissorRect;

    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_cmdQueue;
    ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_cmdList;

    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    ComPtr<ID3D12Resource> m_renderTargets[NUM_SWAP_CHAIN_BUFFERS];

    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    ComPtr<ID3D12RootSignature> m_emptyRootSignature;

    ComPtr<ID3D12PipelineState> m_pipelineState;

    uint32 m_frameIndex = 0;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    // DXGI
    ComPtr<IDXGIFactory2> m_dxgiFactory2;
    ComPtr<IDXGISwapChain3> m_SwapChain3;

    // Queried Info
    uint32 m_rtvDescriptorSize;

public:
    D3D12Context();

    void InitialisePipeline();

    void LoadInitialAssets();

    void Draw();

    void Present();
};