#include "Engine.h"

#include "Renderer.h"


Renderer* gptRenderer;

void EngineInitialise()
{   
    gptRenderer = new Renderer();
}

void EngineDispose()
{
    delete gptRenderer;
}
