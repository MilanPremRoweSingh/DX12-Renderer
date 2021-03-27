#pragma once

void EngineInitialise();

void EngineDispose();

void EngineLog(const char* message);

void EngineUpdate();

void EngineIdle();

void EngineBufferMouseInput(
    const Vector2& input);

float GetCurrentFrameTime();