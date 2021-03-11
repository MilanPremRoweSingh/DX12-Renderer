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

void EngineLog(const char* message)
{
    OutputDebugStringA(message);
}

void Update()
{

}

void EngineIdle()
{
    Update();
    gptRenderer->Render();
}
