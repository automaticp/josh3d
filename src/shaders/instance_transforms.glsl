#ifndef INSTANCE_TRANSFORMS_GLSL
#define INSTANCE_TRANSFORMS_GLSL
// #version 430 core


#ifndef INSTANCE_TRANSFORMS_SSBO_BINDING
#define INSTANCE_TRANSFORMS_SSBO_BINDING 0
#endif


layout (std140, binding = INSTANCE_TRANSFORMS_SSBO_BINDING) restrict readonly
buffer InstanceTransforms {
    mat4 instance_transforms[];
};


#endif
