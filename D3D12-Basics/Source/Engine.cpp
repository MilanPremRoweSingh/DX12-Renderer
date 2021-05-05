#include "Engine.h"

#include "Camera.h"
#include "Scene.h"
#include "Shell.h"

#include "Renderer/Renderer.h"

#include <windows.h>
#include <chrono>

#define CAMERA_MOVE_SPEED 10.0f

Renderer* g_pRenderer;
Scene* s_pCurrScene;
Camera camera;


typedef std::chrono::high_resolution_clock HighResClock;

std::chrono::time_point<HighResClock> startTime;
std::chrono::time_point<HighResClock> currentFrameTime;

void EngineInitialise()
{
    g_pRenderer = new Renderer();
    startTime = HighResClock::now();
    currentFrameTime = startTime;

    s_pCurrScene = Scene::Load("../Data/Models/CursedCornell.obj");

    Vector3 eyePos(0, 0, -10);
    Vector3 targetPos;
    Vector3 camUp(0.0f, 1.0f, 0.0f);
    camUp.Normalize();
    camera = Camera(eyePos, targetPos, camUp, 0.01f, 10000.0f, 60.0f, GetWindowAspectRatio());
    g_pRenderer->CameraSet(&camera);
    g_pRenderer->SceneSet(s_pCurrScene);
}

void EngineDispose()
{
    g_pRenderer->FlushGPU();

    delete s_pCurrScene;
    delete g_pRenderer;
}

void EngineLog(const char* message)
{
    OutputDebugStringA(message);
}

void EngineUpdate(float deltaTime)
{

    Vector3 eyePos;
    camera.GetWorldSpacePosition(eyePos);
    eyePos;
    Vector3 targetPos;
    Vector3 camUp(0.0f, 1.0f, 0.0f);
    camera = Camera(-globals.totalMouseDelta.x, -globals.totalMouseDelta.y, 0.0f, eyePos, 0.01f, 100000.0f, 60.0f, GetWindowAspectRatio());

    Vector3 moveDir;
    globals.cameraMoveDirection.Normalize(moveDir);
    moveDir *= CAMERA_MOVE_SPEED * deltaTime;
    camera.Translate(moveDir);

    globals.totalMouseDelta = { fmodf(globals.totalMouseDelta.x, 2.0f* F_PI), fmodf(globals.totalMouseDelta.y, 2.0f * F_PI) };
}

void EngineIdle()
{
    std::chrono::time_point<HighResClock> old = currentFrameTime;
    currentFrameTime = HighResClock::now();
    std::chrono::duration<float> deltaTime = currentFrameTime - old;

    EngineUpdate(deltaTime.count());
    g_pRenderer->Render();
}

float EngineGetCurrTime()
{
    std::chrono::duration<float> tInSeconds = currentFrameTime.time_since_epoch() - startTime.time_since_epoch();
    return tInSeconds.count();
}

