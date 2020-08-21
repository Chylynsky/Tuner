#pragma once

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


#include <iterator>
#include <cmath>
#include <type_traits>
#include <limits>
#include <cstdint>
#include <algorithm>
#include <execution>