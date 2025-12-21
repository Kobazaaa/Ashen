#version 450 core

layout(push_constant) uniform PushConstants
{
    float exposure;
} pc;


layout(set = 0, binding = 0) uniform sampler2D Render;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 hdrColor = texture(Render, fragTexCoord);
	outColor = vec4(1.0 - exp(hdrColor.rgb * -pc.exposure), hdrColor.a);
}
