#pragma once

class D3D12Core;
struct RenderConstants;
struct RenderConstantEntry;
struct RenderContext;
struct ConstantDataEntry;

#define NUM_SWAP_CHAIN_BUFFERS 2
// For now assume all shaders are in the same file
#define SHADER_FILE L"../Shaders/Shaders.hlsl"

class Renderer 
{
public:
    Renderer();
    ~Renderer();

    void Render();

    void ConstantDataSetEntry(
        const ConstantDataEntry& entry,
        void* data);
    
    void ConstantDataFlush(
        void);

private:

    void ConstantDataInitialise();
    void ConstantDataDispose();

    D3D12Core* m_core;   
    RenderContext* m_context;
};

