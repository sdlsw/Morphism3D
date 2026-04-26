#version 450

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
} camMats;

layout(push_constant) uniform PushConstants {
    mat4 modelTransform;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = camMats.proj * camMats.view * modelTransform * vec4(inPosition, 1.0);
    fragColor = inColor;
}
