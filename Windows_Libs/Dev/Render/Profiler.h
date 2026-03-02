#pragma once

// enable only if you know what you're doing
// requires linking Minecraft.Client against Ws2_32.lib
// stats are available at http://127.0.0.1:1338/

//#define ENABLE_PROFILING

#ifdef ENABLE_PROFILING
#include "microprofile/microprofile.h"
#define PROFILER_SCOPE(group, name, color) MICROPROFILE_SCOPEI(group, name, color);
#else
#define PROFILER_SCOPE(group, name, color) ((void)0);
#endif