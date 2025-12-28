#version 450
#extension GL_GOOGLE_include_directive : require

layout(push_constant) uniform PushConstants
{
    mat4 view;
    mat4 proj;
} pc;

#include "Helper_Scattering.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outRayleighColor;
layout(location = 1) out vec3 outMieColor;
layout(location = 2) out vec3 outDirectionToCam;


// This shader is used to render the sky dome when the camera is in the atmosphere
// This means that the ray along which we will sample starts at the camera and ends at the current vertex
void main()
{
    // Transform the current vertex position to a world-size position instead of the scaled down render version
    float scaledHeight = planetRadius + (length(inPosition) - renderRadius) / (renderThickness) * atmosphereThickness;
    vec3 scaledPos = normalize(inPosition) * scaledHeight;

    // Get the ray from the Camera to the current Vertex,
    // the length of this ray is the far point of the ray passing through the atmosphere
    vec3 startPos = cameraPos;
    vec3 endPos = scaledPos;
    vec3 ray = endPos - startPos;
    float farDistance = length(ray);
    ray /= farDistance;

    // Initialize the scattering loop variables
    float travelDistance = farDistance;
    float sampleLength = travelDistance / sampleCount;
    vec3 sampleRay = ray * sampleLength;
    vec3 samplePoint = startPos + sampleRay * 0.5;

    // Loop through the sample points
    vec3 frontColorR = vec3(0);
    vec3 frontColorM = vec3(0);
    for(int i = 0; i < sampleCount; ++i)
    {
        // Calculate the sample depth for rayleigh and mie
        float sampleHeightOffGround = length(samplePoint) - planetRadius;

        float sampleDepthR = DensityFunction(sampleHeightOffGround, rayleighScaleHeight);
        float sampleDepthM = DensityFunction(sampleHeightOffGround, mieScaleHeigh);

        // Calculate the attenuation
        vec3 exitPos = samplePoint + lightDir * DistanceToAtmosphereExit(samplePoint, lightDir);
        vec3 attenuation = CalculateAttenuation(startPos, samplePoint);
        attenuation *= CalculateAttenuation(samplePoint, exitPos);

        // Add Color
        frontColorR += sampleDepthR * attenuation * sampleLength;
        frontColorM += sampleDepthM * attenuation * sampleLength;

        // Advance to next sample
        samplePoint += sampleRay;
    }

    // Finally, scale the Mie and Rayleigh Colors
    gl_Position = pc.proj * pc.view * vec4(inPosition, 1.0);
    
    outRayleighColor = sunIntensity * betaR * INV_FOUR_PI * frontColorR;
    outMieColor = sunIntensity * betaM * INV_FOUR_PI * frontColorM;

    outDirectionToCam = cameraPos - scaledPos;
}
