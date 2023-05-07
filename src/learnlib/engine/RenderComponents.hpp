#pragma once
#include "GLObjects.hpp"
#include "Shared.hpp"




namespace learn::components {

// WIP
// I'll slowly fill out this with wrappers, tags and aliases
// to have a semi-standard list of components used in rendering


struct Skybox {
    Shared<Cubemap> cubemap;
};



} // namespace learn::components
