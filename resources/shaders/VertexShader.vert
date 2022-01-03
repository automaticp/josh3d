#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalModel;


out vec2 texCoord;
out vec3 normal;
out vec3 fragPos;

void main() {

	fragPos = vec3(model * vec4(aPos, 1.0f)); 
	gl_Position = projection * view * vec4(fragPos, 1.0f);
	texCoord = aTexCoord;
	normal = normalModel * aNormal;

}