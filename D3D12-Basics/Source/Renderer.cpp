#include "Renderer.h"

#include "D3D12Context.h"

void Renderer::Render()
{
    ptContext->Draw();
}

Renderer::Renderer()
{
    ptContext = new D3D12Context();
}

Renderer::~Renderer()
{
    delete ptContext;
}