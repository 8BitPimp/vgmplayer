#pragma once

#ifdef _DEBUG
#define assert(EXP, ...) { if (!(EXP)) __debugbreak(); }
#else
#include <assert.h>
#endif
