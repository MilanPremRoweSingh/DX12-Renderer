#pragma once
#include "EnginePCH.h"

struct Globals
{
    bool fD3DDebug = false;
    bool fGPUValidation = false;

    Vector2 totalMouseDelta = {0.0f, 0.0f};
    Vector3 cameraMoveDirection;
};

extern Globals globals;