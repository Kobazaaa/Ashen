#version 450

layout(set = 0, binding = 1) uniform Parameters
{
    float n;
};


layout(location = 0) in vec3 fragColor0;
layout(location = 1) in vec3 fragColor1;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(fragColor0 + 0.25 * fragColor1, 1.0);
}
