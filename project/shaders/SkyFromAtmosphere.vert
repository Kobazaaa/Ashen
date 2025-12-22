#version 450
#extension GL_GOOGLE_include_directive : require
#include "Helper_Scattering.glsl"

layout(push_constant) uniform PushConstants
{
    mat4 view;
    mat4 proj;
} pc;

layout(set = 0, binding = 0) uniform Parameters
{
    vec3 cameraPos;			        // current camera pos
    float cameraHeight;				// current camera height
    
    vec3 lightDir;				    // direction of the sunlight
    float cameraHeight2;			// cameraHeight^2
    
    vec3 invWaveLength;		        // 1 / (wavelength^4) for RGB
    float sampleCount;		        // nr of samples along the ray
    
    float outerRadius;				// outer atmosphere radius
    float outerRadius2;				// outerRadius^2
    float innerRadius;				// inner planetary radius
    float innerRadius2;				// innerRadius^2
    
    float scale;					// 1 / (outerRadius - innerRadius)
    float scaleDepth;				// scale depth (the altitude at which the average atmospheric density is found)
    float scaleOverScaleDepth;		// scale / scaleDepth
    float invScaleDepth;		    // inverse of scale depth
    
    float krESun;					// Kr * ESun
    float kmESun;					// Km * ESun
    float kr4PI;					// Kr * 4 * PI
    float km4PI;					// Km * 4 * PI
};

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
    vec3 ray = endPos - cameraPos;
    float farDistance = length(ray);
    ray /= farDistance;

    // Calculate the ray's starting position
    float heightOffGround = cameraHeight - innerRadius;
    float depth = exp(-heightOffGround * scaleOverScaleDepth);

    float startAngle = dot(ray, startPos) / cameraHeight;
    float startOffset = depth * Scale(startAngle, scaleDepth);

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
        float sampleHeight = length(samplePoint);
        float sampleHeightOffGround = sampleHeight - innerRadius;
        float sampleDepth = exp(-sampleHeightOffGround * scaleOverScaleDepth);

        float lightAngle = dot(lightDir, samplePoint) / sampleHeight;
        float cameraAngle = dot(ray, samplePoint) / sampleHeight;

        float scatter = (startOffset + sampleDepth * (Scale(lightAngle, scaleDepth) - Scale(cameraAngle, scaleDepth)));
        vec3 attentuation = exp(-scatter * (invWaveLength * kr4PI + km4PI));

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
