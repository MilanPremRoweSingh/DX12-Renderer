#pragma once

#include "d3dx12.h"
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#define ASSERT_SUCCEEDED(x) {HRESULT result = x; assert(SUCCEEDED(result));} 