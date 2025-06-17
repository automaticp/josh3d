#ifndef CAMERA_GLSL
#define CAMERA_GLSL
// #version 430 core


struct Camera
{
    vec3  position_ws;
    float z_near;
    float z_far;
    mat4  view;
    mat4  proj;
    mat4  projview;
    mat4  inv_view;
    mat3  normal_view;
    mat4  inv_proj;
    mat4  inv_projview;
};


#endif
