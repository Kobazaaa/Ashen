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
    // Get the ray from the Camera to the current Vertex,
    // the length of this ray is the far point of the ray passing through the atmosphere
    vec3 startPos = cameraPos;
    vec3 endPos = inPosition;
    vec3 ray = endPos - startPos;
    float farDistance = length(ray);
    ray /= farDistance;

    // Calculate the ray's starting position
    float startDepth = ComputeOpticalDepth(ray, startPos, scaleDepth);

    // Initialize the scattering loop variables
    float travelDistance = farDistance;
    float sampleLength = travelDistance / sampleCount;
    float scaledLength = sampleLength * scale;
    vec3 sampleRay = ray * sampleLength;
    vec3 samplePoint = startPos + sampleRay * 0.5;

    // Loop through the sample points
    vec3 frontColor = vec3(0);
    for(int i = 0; i < sampleCount; ++i)
    {
        // Calculate the sample depth
        float sampleHeightOffGround = length(samplePoint) - innerRadius;
        float normalizedHeight = sampleHeightOffGround * scale;
        float sampleDepth = DensityFunction(normalizedHeight, scaleDepth);

        float lightDepth = ComputeOpticalDepth(lightDir, samplePoint, scaleDepth, sampleDepth);
        float cameraDepth = ComputeOpticalDepth(ray, samplePoint, scaleDepth, sampleDepth);

        float scatter = (startDepth + (lightDepth - cameraDepth));

        vec3 attentuation = exp(-(
                    scatter * invWaveLength * kr4PI + 
                    scatter * km4PI +
                    scatter * kOzoneExt));

        // Add Color
        frontColor += attentuation * (sampleDepth * scaledLength);

        // Advance to next sample
        samplePoint += sampleRay;
    }

    // Finally, scale the Mie and Rayleigh Colors
    gl_Position = pc.proj * pc.view * vec4(inPosition, 1.0);

    outRayleighColor = frontColor * (invWaveLength * krESun);
    outMieColor = frontColor * kmESun;

    outDirectionToCam = cameraPos - inPosition;
}
