#ifndef CAMERA_UBO_GLSL
#define CAMERA_UBO_GLSL
// #version 430 core
// #extension GL_GOOGLE_include_directive : enable
#include "camera.glsl"


layout (std140, binding = 0)
uniform CameraBlock {
    Camera camera;
};


#endif
