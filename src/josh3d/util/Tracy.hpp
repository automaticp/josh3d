#pragma once
#ifdef TRACY_ENABLE
#include <glbinding/gl/gl.h>
using namespace gl; // Sadly, it has to be done.
#endif
#include <tracy/Tracy.hpp>       // IWYU pragma: export
#include <tracy/TracyOpenGL.hpp> // IWYU pragma: export


#define ZS ZoneScoped
#define ZSN(StaticName) ZoneScopedN(StaticName)
#define ZSGPUN(StaticName) TracyGpuZone(StaticName)
#define ZSCGPUN(StaticName) ZSN(StaticName); ZSGPUN(StaticName)

// TODO: Instrumenting the coroutines. Sounds like a pain...
