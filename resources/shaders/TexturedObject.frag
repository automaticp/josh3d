#version 330 core

in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

uniform vec3 lightColor;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform vec3 lightPos;
uniform vec3 camPos;

out vec4 fragColor;

void main() {
   float ambientStrength = 0.1f;
   vec3 ambientColor = ambientStrength * lightColor;
   vec4 zeroLightColor = texture(texture1, texCoord);
   //mix(texture(texture1, texCoord),  texture(texture2, vec2(1.0f - texCoord.s, texCoord.t)), 0.0f);
   
   vec3 norm = normalize(normal);
   vec3 lightVec = lightPos - fragPos;
   vec3 lightDir = normalize(lightVec);
   
   float lightDistance = length(lightVec);
   float distanceWeight = 1.0f / (0.1f * lightDistance * lightDistance);

   float diffuseStrength = max(dot(norm, lightDir), 0.0f);
   vec3 diffuseColor = diffuseStrength * lightColor * distanceWeight;
   
   float specularStrength = 0.5f;
   vec3 viewDir = normalize(camPos - fragPos);
   vec3 reflectionDir = reflect(-lightDir, norm);
   vec3 specularColor = specularStrength * pow(max(dot(viewDir, reflectionDir), 0.0f), 64.0f) * lightColor * distanceWeight;

   fragColor = vec4(ambientColor + diffuseColor + specularColor, 1.0f) * zeroLightColor;
   
}