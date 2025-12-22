#version 450

layout(set = 0, binding = 1) uniform Parameters
{
    float n;
};


layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inAttenuation;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(inColor + 0.25 * inAttenuation, 1.0);
}
