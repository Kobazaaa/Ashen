#version 450

layout(push_constant) uniform PushConstants
{
    mat4 view;
    mat4 proj;
} pc;

layout(set = 0, binding = 0) uniform TestParams
{
	float eT;
} time;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main()
{
    gl_Position = pc.proj * pc.view * vec4(inPosition, 1.0);
    vec3 timeOffset = sin(vec3(time.eT, time.eT * 1.3, time.eT * 1.7)) * 0.5 + 0.5;
    fragColor = inColor * timeOffset;
}
