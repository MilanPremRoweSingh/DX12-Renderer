#pragma once

struct GraphicsContext;

class Renderer 
{
public:
    Renderer();
    ~Renderer();

private:

    GraphicsContext* ptContext;
};