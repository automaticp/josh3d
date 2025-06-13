#pragma once
#include "Semantics.hpp"


namespace josh::globals {

/*
Initialize the global defaults.
Must be done right after creating the OpenGL context.
*/
void init_all();

/*
Clear out all the global pools and textures.
Must be done before destroying the OpenGL context.
*/
void clear_all();

/*
RAII wrapper for initialization and cleanup of globals.
Must be constructed right after creating the OpenGL context.

Use either manual init_all() and clear_all() or this. Not both.
*/
struct RAIIContext
    : Immovable<RAIIContext>
{
    RAIIContext() { init_all(); }
    RAIIContext(const RAIIContext&) = delete;
    RAIIContext(RAIIContext&&) = delete;
    RAIIContext& operator=(const RAIIContext&) = delete;
    RAIIContext& operator=(RAIIContext&&) = delete;
    ~RAIIContext() { clear_all(); }
};


} // namespace josh::globals
