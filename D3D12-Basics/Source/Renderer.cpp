#include "Renderer.h"

#include "Shell.h"

#include <wrl/client.h>
#include "d3d12.h"
#include "dxgi1_3.h"

using Microsoft::WRL::ComPtr;

struct GraphicsContext
{
    ComPtr<ID3D12Device> pDevice;
    ComPtr<ID3D12CommandQueue> pCmdQueue;

    // DXGI
    ComPtr<IDXGIFactory2> pDXGIFactory;
    ComPtr<IDXGISwapChain1> pSwapChain;

};

Renderer::Renderer()
{
    ptContext = new GraphicsContext();

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
        HRESULT result = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&ptContext->pDevice));
        ASSERT(SUCCEEDED(result));
    }

    // Create Command Queue
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

        HRESULT result = ptContext->pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&ptContext->pCmdQueue));
        ASSERT(SUCCEEDED(result));
    }

    // Create Swapchain
    {
        HRESULT result = CreateDXGIFactory2(0, IID_PPV_ARGS(&ptContext->pDXGIFactory));
        ASSERT(SUCCEEDED(result));

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = GetWindowWidth();
        desc.Height = GetWindowHeight();
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Stereo = false;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;    
        desc.Scaling = DXGI_SCALING_NONE;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        result = ptContext->pDXGIFactory->CreateSwapChainForHwnd(
            ptContext->pCmdQueue.Get(),
            GetNativeViewHandle(),
            &desc,
            NULL,
            NULL,
            &ptContext->pSwapChain);
        ASSERT(SUCCEEDED(result));
    }
}


Renderer::~Renderer()
{
    delete ptContext;
}