#pragma once

#include "Renderer/Renderer.h"

void EngineInitialise();

void EngineDispose();

void EngineLog(const char* message);

void EngineUpdate(float deltaTime);

void EngineIdle();

float EngineGetCurrTime();

extern Renderer* g_pRenderer;