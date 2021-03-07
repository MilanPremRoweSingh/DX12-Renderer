#pragma once

class GraphicsContext;

class Renderer 
{
public:
    Renderer();
    ~Renderer();

private:

    D3D12Context* ptContext;
};