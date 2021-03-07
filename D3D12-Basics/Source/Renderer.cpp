#include "Renderer.h"

#include "d3d12.h"
#include "d3d12sdklayers.h"

struct GraphicsContext
{
    ID3D12Device* pDevice;
    ID3D12CommandQueue* pCmdQueue;
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
}