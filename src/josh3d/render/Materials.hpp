#pragma once
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "Resource.hpp"
#include "Scalars.hpp"


namespace josh {

/*
Material spec for the "Classic" Phong shading model.

NOTE: Not adding `color_factor`, `specular_factor`, etc.
because that's extra data to send when "instancing" and
I don't want to bother right now.

HMM: Textures are stored as-is, but it would probably
be better to have a specializaed storage for them too.
*/
struct MaterialPhong
{
    SharedConstTexture2D diffuse;   // [sRGB|sRGBA] Diffuse color.
    SharedConstTexture2D normal;    // [RGB] Tangent space normal map.
    SharedConstTexture2D specular;  // [R] Specular contribution factor.
    float                specpower; // That one parameter that nobody specifies.

    // TODO: This is pretty dumb, but is needed in the current system.
    ResourceUsage        diffuse_usage  = {};
    ResourceUsage        normal_usage   = {};
    ResourceUsage        specular_usage = {};

    // TODO: No idea how, but this is better be "moved outside" somehow.
    uintptr              aba_tag = {};
};


/*
Old material spec below. This will take a while to replace fully.
*/
struct MaterialDiffuse
{
    SharedConstTexture2D texture;
    ResourceUsage        usage;
    uintptr              aba_tag = {};
};

struct MaterialSpecular
{
    SharedConstTexture2D texture;
    ResourceUsage        usage;
    GLfloat              shininess = 128.f;
    uintptr              aba_tag   = {};
};

struct MaterialNormal
{
    SharedConstTexture2D texture;
    ResourceUsage        usage;
    uintptr              aba_tag = {};
};


} // namespace josh
