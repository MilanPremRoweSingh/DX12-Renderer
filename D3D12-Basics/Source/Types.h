#pragma once

typedef unsigned int uint32;
typedef int int32;
typedef unsigned short uint16;
typedef short int16;

#ifdef _DEBUG
#define ASSERTIONS_ENABLED
#endif

#ifdef ASSERTIONS_ENABLED
#define ASSERT(x) assert(x) 
#else
#define ASSERT(X) 
#endif