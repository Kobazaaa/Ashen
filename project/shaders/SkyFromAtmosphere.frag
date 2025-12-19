#version 450
#extension GL_GOOGLE_include_directive : require
#include "Functions.glsl"

layout(set = 0, binding = 1) uniform Parameters
{
    vec3 lightDir;				    // direction of the sunlight
    float g;						// constant that affects symmetry of the scattering
    float g2;						// g^2
};

layout(location = 0) in vec3 fragColor0;
layout(location = 1) in vec3 fragColor1;
layout(location = 2) in vec3 fragCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    // Angle between light and -view direction
    float cosine = dot(lightDir, fragCoord) / length(fragCoord);
    float cosine2 = cosine * cosine;

    vec3 c = fragColor0 
            + GetMiePhase(cosine, cosine2, g, g2) * fragColor1;
    
    outColor = vec4(c, c.b);
}
