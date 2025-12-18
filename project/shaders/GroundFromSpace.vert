#version 450
#extension GL_GOOGLE_include_directive : require
#include "Functions.glsl"

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

layout(location = 0) out vec3 fragColor0;
layout(location = 1) out vec3 fragColor1;


// This shader is used to render the ground when the camera is in space
// This means that the ray along which we will sample starts at the intersection point with the skydome
// and ends at the current vertex
void main()
{
    // Get the ray from the Camera to the current Vertex,
    // the length of this ray is the far point of the ray passing through the atmosphere
    vec3 startPos = cameraPos;
    vec3 endPos = inPosition;
    vec3 ray = endPos - startPos;
    float farDistance = length(ray);
    ray /= farDistance;

    // Calculate where they ray first intersects with the atmosphere
    float nearDistance = GetNearIntersection(cameraPos, ray, cameraHeight2, outerRadius2);
    startPos = cameraPos + ray * nearDistance;
    
    float heightOffGround = outerRadius - innerRadius;
    float depth = exp(-heightOffGround / scaleDepth);
    
    float cameraAngle = dot(-ray, endPos) / length(endPos);
    float cameraScale = Scale(cameraAngle, scaleDepth);
    float cameraOffset = depth * cameraScale;

    float lightAngle = dot(lightDir, endPos) / length(endPos);
    float lightScale = Scale(lightAngle, scaleDepth);

    float temp = lightScale + cameraScale;

    // Initialize the scattering loop variables
    float travelDistance = farDistance - nearDistance;
    float sampleLength = travelDistance / sampleCount;
    float scaledLength = sampleLength * scale;
    vec3 sampleRay = ray * sampleLength;
    vec3 samplePoint = startPos + sampleRay * 0.5;

    // Loop through the sample points
    vec3 frontColor = vec3(0);
    vec3 attentuation;
    for(int i = 0; i < sampleCount; ++i)
    {
        // Calculate the sample depth
        float sampleHeight = length(samplePoint);
        float sampleHeightOffGround = sampleHeight - innerRadius;
        float sampleDepth = exp(-sampleHeightOffGround * scaleOverScaleDepth);

        float scatter = sampleDepth * temp - cameraOffset;
        attentuation = exp(-scatter * (invWaveLength * kr4PI + km4PI));
        
        // Add Color
        frontColor += attentuation * (sampleDepth * scaledLength);

        // Advance to the next sample point
        samplePoint += sampleRay;
    }

    // Finally, scale the Mie and Rayleigh Colors
    gl_Position = pc.proj * pc.view * vec4(inPosition, 1.0);

    fragColor0 = frontColor * (invWaveLength * krESun + kmESun);
    fragColor1 = attentuation;
}
