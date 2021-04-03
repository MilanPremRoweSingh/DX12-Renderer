#pragma once

#define NOMINMAX

#include "Types.h"

#include <assert.h>
#include <windows.h>
#include <SimpleMath.h>

typedef DirectX::SimpleMath::Matrix Matrix4x4;
typedef DirectX::SimpleMath::Vector4 Vector4;
typedef DirectX::SimpleMath::Vector3 Vector3;
typedef DirectX::SimpleMath::Vector2 Vector2;

#define F_PI 3.14159265358979323846264338327950288f
#define F_DEG_TO_RAD (F_PI / 180.f)
#define F_RAD_TO_DEG (1.0f / DEG_TO_RAD)

#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)

#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)

#ifdef _DEBUG
#define ASSERTIONS_ENABLED
#endif

#ifdef ASSERTIONS_ENABLED
#define ASSERT(x) assert(x) 
#else
#define ASSERT(X) 
#endif

#include "Utils.h"
#include "Globals.h"
