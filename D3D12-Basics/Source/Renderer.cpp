#include "Renderer.h"

#include "Shell.h"

#include <wrl/client.h>

#include "d3d12.h"
#include "d3dx12.h"

#include "dxgi1_6.h"

#define NUM_SWAP_CHAIN_BUFFERS 2

#define ASSERT_SUCCEEDED(x) {HRESULT result = x; assert(SUCCEEDED(result));} 

using Microsoft::WRL::ComPtr;

class D3D12Context
{
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_cmdQueues;
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    ComPtr<ID3D12Resource> m_rtvDescriptors[NUM_SWAP_CHAIN_BUFFERS];

    // DXGI
    ComPtr<IDXGIFactory2> m_dxgiFactory2;
    ComPtr<IDXGISwapChain3> m_SwapChain3;
    uint32 m_frameIndex = 0;

    // Queried Info
    uint32 m_rtvDescriptorSize;

public:
    D3D12Context();
};

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

    // Create Device
    {
        ASSERT_SUCCEEDED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));
    }

    // Create Command Queue
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

        ASSERT_SUCCEEDED(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_cmdQueues)));
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
            m_cmdQueues.Get(),
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
            ASSERT_SUCCEEDED(m_SwapChain3->GetBuffer(m_frameIndex, IID_PPV_ARGS(&m_rtvDescriptors[i])));
            m_device->CreateRenderTargetView(m_rtvDescriptors[i].Get(), NULL, handle);
            handle.Offset(1, m_rtvDescriptorSize);
        }
    }
    
}

Renderer::Renderer()
{
    ptContext = new GraphicsContext();

}


Renderer::~Renderer()
{
    delete ptContext;
}