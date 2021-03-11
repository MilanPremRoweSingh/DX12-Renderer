#pragma once

class D3D12Context;

class Renderer 
{
public:
    Renderer();
    ~Renderer();

    void Render();

private:

    D3D12Context* ptContext;
};

#define NUM_SWAP_CHAIN_BUFFERS 2
// For now assume all shaders are in the same file
#define SHADER_FILE L"../Shaders/Shaders.hlsl"

