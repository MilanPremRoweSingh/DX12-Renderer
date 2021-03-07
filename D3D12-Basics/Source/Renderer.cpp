#include "Renderer.h"

#include "d3d12.h"
#include "d3d12sdklayers.h"

struct GraphicsContext
{
    ID3D12Device* device;
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
            debugController->SetEnableGPUBasedValidation(true);
            debugController->Release();
        }
    }

    HRESULT result = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&ptContext->device));
    ASSERT(SUCCEEDED(result));
}