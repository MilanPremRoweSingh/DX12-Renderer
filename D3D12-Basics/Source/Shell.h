#pragma once
#include <windows.h>

// Use hardcoded width and height for now
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

void ShellInitialise(
    HINSTANCE hInstance);

void ProcessWindowMessages();

int32 GetWindowWidth();
int32 GetWindowHeight();

bool WindowExists();