#version 450

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
} camData;

layout(push_constant) uniform PushConstants {
    mat4 modelTransform;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;

void main() {
    vec4 worldPos = modelTransform * vec4(inPosition, 1.0);
    gl_Position = camData.proj * camData.view * worldPos;
    fragPosition = worldPos.xyz;
    fragColor = inColor;
    fragNormal = inNormal;
}
