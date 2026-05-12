#version 450

layout(set = 0, binding = 0) uniform CameraBuffer {
    layout(offset = 128) vec3 pos;
} camData;

layout(set = 0, binding = 1) uniform LightBuffer {
    vec3 pos;
    vec3 color;
    float _mix;
} lightData;

layout(push_constant) uniform PushConstants {
    layout(offset = 64) vec3 ambientStrength;
    vec3 diffuseStrength;
    vec3 specularStrength;
    float shine;
};

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDirection = normalize(fragPos - lightData.pos);

    vec3 ambient = ambientStrength * lightData.color;

    float baseDiffuse = max(dot(fragNormal, -lightDirection), 0.0);
    vec3 diffuse = baseDiffuse * diffuseStrength * lightData.color;

    vec3 viewDir = normalize(camData.pos - fragPos);
    vec3 reflectDir = reflect(lightDirection, fragNormal);
    float baseSpecular = pow(max(dot(viewDir, reflectDir), 0.0), shine);
    vec3 specular = baseSpecular * specularStrength * lightData.color;

    vec3 lightFactor = ambient + diffuse + specular;
    vec3 lightMixed = mix(vec3(1.0f, 1.0f, 1.0f), lightFactor, lightData._mix);
    outColor = vec4(lightMixed * fragColor, 0.0);
}
