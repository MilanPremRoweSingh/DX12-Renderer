#include "Engine.h"

#include "Renderer.h"
#include "Camera.h"
#include "Shell.h"

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

    float radius = 10.0f;
    float time = GetCurrentFrameTime();
    Vector3 eyePos(radius * cosf(time), 0.0f, radius * sinf(time));
    Vector3 targetPos;
    Vector3 camUp(0.0f, 1.0f, 0.0f);
    camUp.Normalize();
    gptRenderer->CameraSet(Camera(eyePos, targetPos, camUp, 0.1f, 100.0f, 90.0f, GetWindowAspectRatio()));
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
