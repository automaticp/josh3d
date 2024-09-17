#ifndef CAMERA_UBO_GLSL
#define CAMERA_UBO_GLSL
// #version 430 core
// #extension GL_GOOGLE_include_directive : enable
#include "camera.glsl"


#ifndef CAMERA_UBO_BINDING
#define CAMERA_UBO_BINDING 0
#endif


layout (std140, binding = CAMERA_UBO_BINDING)
uniform CameraBlock {
    Camera camera;
};


#endif
