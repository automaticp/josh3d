#version 330 core

in vec3 normal;
in vec3 fragPos;

struct Material {
    vec3 ambient;       // color reflected under ambient lighting
    vec3 diffuse;       // color reflected under diffuse lighting
    vec3 specular;      // color of the specular highlight
    float shininess;    // defines the scattering radius of the highlight
};

uniform Material material;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

out vec4 fragColor;

void main() {

    // -- Geometry --
    // normal unit vector of fragment
    vec3 normalDir = normalize(normal);
    // light to fragment vector
    vec3 lightVec = lightPos - fragPos;                 // vector from light source to fragment
    vec3 lightDir = normalize(lightVec);                // direction from light to fragment
    float lightDistance = length(lightVec);
    // 1/r^2 intensity scaling
    float distanceWeight = 1.0f / (0.1f * lightDistance * lightDistance);
    // reflection direction
    vec3 viewDir = normalize(camPos - fragPos);         // direction from camera to fragment
    vec3 reflectionDir = reflect(-lightDir, normalDir); // reflection direction around a normal
    float specularAlignment = max(dot(viewDir, reflectionDir), 0.0f);   // how close camera is to the reflection line

    // -- Color --
    // weights
    const float ambientStrength = 0.1f;     // (could be uniform)
    float diffuseStrength = max(dot(normalDir, lightDir), 0.0f);
    const float specularStrength = 0.8f;
    // ambient color
    vec3 ambientColor = ambientStrength * material.ambient;
    // diffuse color
    vec3 diffuseColor = diffuseStrength * material.diffuse * lightColor;
    // specular color
    vec3 specularColor = specularStrength * pow(specularAlignment, material.shininess) * lightColor;

    // C_ambient + k * [(C_diffuse + C_specular) / R^2]
    fragColor = vec4(ambientColor + distanceWeight * (diffuseColor + specularColor), 1.0f);
}
