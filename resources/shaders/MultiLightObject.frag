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
#define NUM_POINT_LIGHTS 4
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform SpotlightLight spotLight;

out vec4 fragColor;


struct TextureColor {
    vec3 diffuse;
    vec3 specular;
};

struct LightParams {
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float specularAlignment;
    float distanceFactor;
};

LightParams getLightParams(vec3 normalDir, vec3 lightDir, vec3 viewDir, float lightDistance, Attenuation att);
vec3 getTotalLightColor(vec3 lightColor, const LightParams lp, TextureColor texColor);


vec3 getLightDirectional(DirectionalLight light, vec3 normalDir, vec3 viewDir, TextureColor texColor) {

    vec3 lightDir = normalize(-light.direction); // direction from fragment to light ???
    Attenuation noAtt = Attenuation(1.0f, 0.0f, 0.0f); // no distance scaling for directional light
    LightParams lp = getLightParams(normalDir, lightDir, viewDir, 0.0f, noAtt);

    return getTotalLightColor(light.color, lp, texColor);
}

vec3 getLightPoint(PointLight light, vec3 normalDir, vec3 viewDir, TextureColor texColor) {

    vec3 lightVec = light.position - fragPos; // vector from light source to fragment
    vec3 lightDir = normalize(lightVec); // direction from fragment to light
    float lightDistance = length(lightVec);
    LightParams lp = getLightParams(normalDir, lightDir, viewDir, lightDistance, light.attenuation);

    return getTotalLightColor(light.color, lp, texColor);
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
        result = edgeFactor * getTotalLightColor(light.color, lp, texColor);
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

    for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
        result += getLightPoint(pointLights[i], normalDir, viewDir, texColor);
    }

    result += getLightSpotlight(spotLight, normalDir, viewDir, texColor);

    fragColor = vec4(result, 1.0f);
}

LightParams getLightParams(vec3 normalDir, vec3 lightDir, vec3 viewDir, float lightDistance, Attenuation att) {
    LightParams lp;
    lp.diffuseStrength = max(dot(normalDir, lightDir), 0.0f);
    lp.ambientStrength = 0.1f;
    lp.specularStrength = 0.8f;
    vec3 reflectionDir = reflect(-lightDir, normalDir); // reflection direction around a normal
    lp.specularAlignment = max(dot(viewDir, reflectionDir), 0.0f);   // how close camera is to the reflection line
    lp.distanceFactor = 1.0f / (att.constant + att.linear * lightDistance + att.quadratic * (lightDistance * lightDistance));
    return lp;
}

vec3 getTotalLightColor(vec3 lightColor, const LightParams lp, TextureColor texColor) {
    // Colors
    vec3 ambientColor = lp.ambientStrength * texColor.diffuse;
    vec3 diffuseColor = lp.diffuseStrength * texColor.diffuse * lightColor;
    vec3 specularColor = lp.specularStrength * texColor.specular * pow(lp.specularAlignment, material.shininess) * lightColor;
    // FIXME: the ambient color from all sources stacks unconditionally
    return (ambientColor + lp.distanceFactor * (diffuseColor + specularColor));
}

