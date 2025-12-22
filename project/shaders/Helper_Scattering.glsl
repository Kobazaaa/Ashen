layout(set = 0, binding = 0) uniform Parameters
{
    vec3 cameraPos;			        // current camera pos
    float cameraHeight;				// current camera height
    
    vec3 lightDir;				    // direction of the sunlight
    float cameraHeight2;			// cameraHeight^2
    
    vec3 invWaveLength;		        // 1 / (wavelength^4) for RGB
    float sampleCount;		        // nr of samples along the ray
    
    vec3 kOzoneExt;		        	// Ozone Extinction Coefficient
    
    float outerRadius;				// outer atmosphere radius
    float outerRadius2;				// outerRadius^2
    float innerRadius;				// inner planetary radius
    float innerRadius2;				// innerRadius^2
    
    float scale;					// 1 / (outerRadius - innerRadius)
    float scaleDepth;				// scale depth (the altitude at which the average atmospheric density is found)
    
    float krESun;					// Kr * ESun
    float kmESun;					// Km * ESun
    float kr4PI;					// Kr * 4 * PI
    float km4PI;					// Km * 4 * PI
};

float Scale(float cosine, float scaleDepth)
{
	float x = 1.0 - cosine;
	return scaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

float DensityFunction(float heightOffGround, float scaleHeight)
{
    return exp(- heightOffGround / scaleHeight);
}

float ComputeOpticalDepth(vec3 rayDir, vec3 startPos, float scaleDepth)
{
    float height = length(startPos);
    float cosAngle = dot(rayDir, startPos) / height;
    height -= innerRadius;

    float density = DensityFunction(height * scale, scaleDepth);
    float opticalDepth = density * Scale(cosAngle, scaleDepth);

    return opticalDepth;
}
float ComputeOpticalDepth(vec3 rayDir, vec3 startPos, float scaleDepth, float density)
{
    float height = length(startPos);
    float cosAngle = dot(rayDir, startPos) / height;

    float opticalDepth = density * Scale(cosAngle, scaleDepth);
    return opticalDepth;
}

