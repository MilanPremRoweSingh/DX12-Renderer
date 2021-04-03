#include "Engine.h"

#include "Renderer.h"
#include "Camera.h"
#include "Shell.h"

#include <windows.h>
#include <chrono>

Renderer* gptRenderer;

Vector2 bufferedMouseInput;

Camera camera;

typedef std::chrono::high_resolution_clock HighResClock;

std::chrono::time_point<HighResClock> tStartTime;
std::chrono::time_point<HighResClock> tCurrentFrameTime;

void EngineInitialise()
{   
    gptRenderer = new Renderer();
    tStartTime = HighResClock::now();
    tCurrentFrameTime = tStartTime;

    Vector3 eyePos(0, 0.0f, -10);
    Vector3 targetPos;
    Vector3 camUp(0.0f, 1.0f, 0.0f);
    camUp.Normalize();
    camera = Camera(eyePos, targetPos, camUp, 0.1f, 100.0f, 90.0f, GetWindowAspectRatio());
    gptRenderer->CameraSet(&camera);
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

    Vector3 eyePos(0, 0.0f, -10);
    Vector3 targetPos;
    Vector3 camUp(0.0f, 1.0f, 0.0f);
    camera = Camera(bufferedMouseInput.x, -bufferedMouseInput.y, 0.0f, eyePos, 0.1f, 100.0f, 90.0f, GetWindowAspectRatio());

}

void EngineIdle()
{
    EngineUpdate();
    gptRenderer->Render();
}


void EngineBufferMouseInput(
    const Vector2& input)
{
    bufferedMouseInput += input;
}

float GetCurrentFrameTime()
{
    std::chrono::duration<float> tInSeconds = tCurrentFrameTime.time_since_epoch() - tStartTime.time_since_epoch();
    return tInSeconds.count();
}
