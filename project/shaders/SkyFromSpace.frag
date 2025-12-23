#version 450
#extension GL_GOOGLE_include_directive : require
#include "Helper_PhaseFunctions.glsl"

layout(location = 0) in vec3 inRayleighColor;
layout(location = 1) in vec3 inMieColor;
layout(location = 2) in vec3 inDirectionToCam;

layout(location = 0) out vec4 outColor;

void main()
{
    // Angle between light and -view direction
    float cosine = dot(lightDir, inDirectionToCam) / length(inDirectionToCam);
    float cosine2 = cosine * cosine;

    vec3 c = GetRayleighPhase(cosine2) * inRayleighColor 
            + GetMiePhase(cosine, cosine2, g, g2) * inMieColor;

    outColor = vec4(c, c.b);
}
