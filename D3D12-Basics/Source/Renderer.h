#pragma once

class D3D12Context;

class Renderer 
{
public:
    Renderer();
    ~Renderer();

private:

    D3D12Context* ptContext;
};