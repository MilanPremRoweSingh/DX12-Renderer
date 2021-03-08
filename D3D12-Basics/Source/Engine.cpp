#include "Engine.h"

#include "Renderer.h"

#include <windows.h>

Renderer* gptRenderer;

void EngineInitialise()
{   
    gptRenderer = new Renderer();
}

void EngineDispose()
{
    delete gptRenderer;
}

void EngineLog(char* message)
{
    OutputDebugStringA(message);
}
