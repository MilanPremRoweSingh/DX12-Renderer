#include "Engine.h"

#include "Renderer.h"

#include <windows.h>
#include <chrono>

Renderer* gptRenderer;

typedef std::chrono::high_resolution_clock HighResClock;

std::chrono::time_point<HighResClock> tStartTime;
std::chrono::time_point<HighResClock> tCurrentFrameTime;

void EngineInitialise()
{   
    gptRenderer = new Renderer();
    tStartTime = HighResClock::now();
    tCurrentFrameTime = tStartTime;
}

void EngineDispose()
{
    delete gptRenderer;
}

void EngineLog(const char* message)
{
    OutputDebugStringA(message);
}

void EngineUpdate()
{
    tCurrentFrameTime = HighResClock::now();
}

void EngineIdle()
{
    EngineUpdate();
    gptRenderer->Render();
}

float GetCurrentFrameTime()
{
    std::chrono::duration<float> tInSeconds = tCurrentFrameTime.time_since_epoch() - tStartTime.time_since_epoch();
    return tInSeconds.count();
}
