#version 330 core

in vec3 normal;
in vec3 fragPos;
in vec2 texCoord;

uniform vec3 camPos;

struct Material {
    sampler2D diffuse;      // color reflected under diffuse lighting
    sampler2D specular;     // color of the specular highlight
    float shininess;        // defines the scattering radius of the highlight
};

uniform Material material;

// Light sources
struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};

struct DirectionalLight {
    vec3 color;
    vec3 direction;
};

struct PointLight {
    vec3 color;
    vec3 position;
    Attenuation attenuation;
};

struct SpotlightLight {
    vec3 color;
    vec3 position;
    vec3 direction;
    Attenuation attenuation;
    float innerCutoffCos;
    float outerCutoffCos;
};

uniform DirectionalLight dirLight;

#define MAX_POINT_LIGHTS 32
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;

uniform SpotlightLight spotLight;

out vec4 fragColor;


struct TextureColor {
    vec3 diffuse;
    vec3 specular;
};

// Parameters for various geometrical attenuations
struct LightParams {
    float diffuseAlignment;
    float specularAlignment;
    float distanceFactor;
};

// Output strength after accounting for alignment and distance
struct LightStrength {
    float ambient;
    float diffuse;
    float specular;
};

// Initial weights for different light types
struct LightComponentWeights {
    float ambient;
    float diffuse;
    float specular;
};

LightParams getLightParams(vec3 normalDir, vec3 lightDir, vec3 viewDir, float lightDistance, Attenuation att);
LightStrength getLightStrength(const LightComponentWeights lcw, const LightParams lp);
vec3 getTotalLightColor(vec3 lightColor, const LightStrength ls, TextureColor texColor);


vec3 getLightDirectional(DirectionalLight light, vec3 normalDir, vec3 viewDir, TextureColor texColor) {

    vec3 lightDir = normalize(-light.direction); // direction from fragment to light ???
    Attenuation noAtt = Attenuation(1.0f, 0.0f, 0.0f); // no distance scaling for directional light
    LightParams lp = getLightParams(normalDir, lightDir, viewDir, 0.0f, noAtt);

    LightStrength ls = getLightStrength(LightComponentWeights(0.1f, 1.0f, 0.8f), lp);

    return getTotalLightColor(light.color, ls, texColor);
}

vec3 getLightPoint(PointLight light, vec3 normalDir, vec3 viewDir, TextureColor texColor) {

    vec3 lightVec = light.position - fragPos; // vector from light source to fragment
    vec3 lightDir = normalize(lightVec); // direction from fragment to light
    float lightDistance = length(lightVec);
    LightParams lp = getLightParams(normalDir, lightDir, viewDir, lightDistance, light.attenuation);

    LightStrength ls = getLightStrength(LightComponentWeights(0.1f, 1.0f, 0.8f), lp);

    return getTotalLightColor(light.color, ls, texColor);
}

vec3 getLightSpotlight(SpotlightLight light, vec3 normalDir, vec3 viewDir, TextureColor texColor) {
    vec3 lightVec = light.position - fragPos; // vector from light source to fragment
    vec3 lightDir = normalize(lightVec); // direction from fragment to light (NOT direction of source)
    float thetaCos = dot(lightDir, normalize(-light.direction)); // cos(angle) between light.direction and light-to-fragment dir

    vec3 result;
    if (thetaCos > light.outerCutoffCos) {
        float edgeFactor;
        if (thetaCos < light.innerCutoffCos) {
            edgeFactor = clamp(
                (thetaCos - light.outerCutoffCos) / (light.innerCutoffCos - light.outerCutoffCos),
                0.0f, 1.0f
            );
        }
        else { edgeFactor = 1.0f; }

        float lightDistance = length(lightVec);
        LightParams lp = getLightParams(normalDir, lightDir, viewDir, lightDistance, light.attenuation);

        LightStrength ls = getLightStrength(LightComponentWeights(0.1f, 1.0f, 0.8f), lp);

        result = edgeFactor * getTotalLightColor(light.color, ls, texColor);
    }
    else {
        result = vec3(0.0f); // no light ouside the cutoff
    }

    return result;
}




void main() {

    // -- Geometry --
    // normal unit vector of fragment
    vec3 normalDir = normalize(normal);
    // direction from fragment to camera
    vec3 viewDir = normalize(camPos - fragPos);
    TextureColor texColor;
    texColor.diffuse = vec3(texture(material.diffuse, texCoord));
    texColor.specular = vec3(texture(material.specular, texCoord));

    vec3 result = getLightDirectional(dirLight, normalDir, viewDir, texColor);

    for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; ++i) {
        result += getLightPoint(pointLights[i], normalDir, viewDir, texColor);
    }

    result += getLightSpotlight(spotLight, normalDir, viewDir, texColor);

    fragColor = vec4(result, 1.0f);
}

LightParams getLightParams(vec3 normalDir, vec3 lightDir, vec3 viewDir, float lightDistance, Attenuation att) {
    LightParams lp;
    lp.diffuseAlignment = max(dot(normalDir, lightDir), 0.0f); // how `normal` light is to the surface
    vec3 reflectionDir = reflect(-lightDir, normalDir); // reflection direction around a normal
    lp.specularAlignment = max(dot(viewDir, reflectionDir), 0.0f);   // how close camera is to the reflection line
    lp.distanceFactor = 1.0f / (att.constant + att.linear * lightDistance + att.quadratic * (lightDistance * lightDistance));
    return lp;
}

LightStrength getLightStrength(const LightComponentWeights lcw, const LightParams lp) {
    LightStrength ls;
    ls.ambient = lcw.ambient * lp.distanceFactor;
    ls.diffuse = lcw.diffuse * lp.diffuseAlignment * lp.distanceFactor;
    ls.specular = lcw.specular * pow(lp.specularAlignment, material.shininess) * lp.distanceFactor;
    return ls;
}

vec3 getTotalLightColor(vec3 lightColor, const LightStrength ls, TextureColor texColor) {

    vec3 ambientColor = ls.ambient * texColor.diffuse;
    vec3 diffuseColor = ls.diffuse * texColor.diffuse;
    vec3 specularColor = ls.specular * texColor.specular;

    return (lightColor * (ambientColor + diffuseColor + specularColor));
}

