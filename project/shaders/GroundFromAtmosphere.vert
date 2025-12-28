#version 450
#extension GL_GOOGLE_include_directive : require

layout(push_constant) uniform PushConstants
{
    mat4 view;
    mat4 proj;
} pc;

layout(set = 0, binding = 0) uniform Parameters
{
    vec3 lightDir; // direction of to sunlight
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

void main()
{
    gl_Position = pc.proj * pc.view * vec4(inPosition, 1.0);
    outColor = max(0, dot(normalize(inPosition), lightDir)) * inColor;
}
