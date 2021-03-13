#pragma once

#include "Types.h"
#include "Globals.h"

#include <assert.h>
#include <windows.h>
#include <SimpleMath.h>

typedef DirectX::SimpleMath::Matrix Matrix4x4;
typedef DirectX::SimpleMath::Vector4 Vector4;
typedef DirectX::SimpleMath::Vector3 Vector3;

#define F_PI 3.14159265358979323846264338327950288f
#define F_DEG_TO_RAD (F_PI / 180.f)
#define F_RAD_TO_DEG (1.0f / DEG_TO_RAD)