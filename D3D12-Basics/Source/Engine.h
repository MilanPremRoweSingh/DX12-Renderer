#pragma once

void EngineInitialise();

void EngineDispose();

void EngineLog(const char* message);

void EngineUpdate(float deltaTime);

void EngineIdle();

float EngineGetCurrTime();