#pragma once



// Declarations of globals are split between headers
// to avoid circular dependencies that arise
// when chucking a bunch of unrelated classes
// into a single header, and when suddenly one of
// them needs one of the globals.
//
// Include specific Globals* headers.


namespace learn::globals {

// Initialize the global defaults.
// Must be done right after creating the OpenGL context.
void init_all();


// Clear out all the global pools and textures.
// Must be done before destroying the OpenGL context.
void clear_all();


// RAII wrapper for initialization and cleanup of globals.
// Must be constructed right after creating the OpenGL context.
class RAIIContext {
public:
    RAIIContext() { init_all(); }
    RAIIContext(const RAIIContext&) = delete;
    RAIIContext(RAIIContext&&) = delete;
    RAIIContext& operator=(const RAIIContext&) = delete;
    RAIIContext& operator=(RAIIContext&&) = delete;
    ~RAIIContext() { clear_all(); }
};


} // namespace learn::globals
