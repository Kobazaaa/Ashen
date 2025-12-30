layout(set = 0, binding = 0) uniform Parameters
{
    vec3 cameraPos;			        // current camera pos
    float cameraHeight;				// current camera height
    
    vec3 lightDir;				    // direction of to sunlight
    float sampleCount;		        // nr of samples along the ray
    
    vec3 betaR;				        // rayleigh coefficient
    float atmosphereThickness;      // thickness of the atmosphere
    
    vec3 betaM;				        // mie coefficient
    float planetRadius;				// planetary radius
    
    float rayleighScaleHeight;      // scale height rayleigh (the altitude at which the average atmospheric density is found)
    float mieScaleHeigh;            // scale height mie (the altitude at which the average atmospheric density is found)
    float sunIntensity;             // intensity of the sun
    
    float renderRadius;				// the radius of the planet at which it is renderer
    float renderThickness;			// the thickness of the atmosphere at which it is renderer
};

// -- Constants --
const float PI = 3.14159265359;
const float INV_PI = 0.31830988618;
const float FOUR_PI = 12.5663706144;
const float INV_FOUR_PI = 0.07957747154;

// -- Density Function --
float DensityFunction(float heightOffGround, float scaleHeight)
{
    return exp(- heightOffGround / scaleHeight);
}

// -- Attenuation --
vec3 CalculateAttenuation(vec3 start, vec3 end)
{
    // Make the ray
    vec3 ray = end - start;
    float travelDistance = length(ray);
    ray /= travelDistance;

    // Setup loop variables
    float sampleLength = travelDistance / sampleCount;
    vec3 sampleRay = ray * sampleLength;
    vec3 samplePoint = cameraPos + sampleRay * 0.5;

    // -- Intagration time
    vec3 resultRayleigh = vec3(0);
    vec3 resultMie = vec3(0);
    for(int i = 0; i < sampleCount; ++i)
    {
        float height = length(samplePoint) - planetRadius;

        // Calculate density at height
        float dR = DensityFunction(height, rayleighScaleHeight);
        float dM = DensityFunction(height, mieScaleHeigh);
        
        // sum up
        resultRayleigh  += dR * sampleLength;
        resultMie       += dM * sampleLength;

        samplePoint += sampleRay;
    }

    // calibrate with coefficients
    resultRayleigh *= betaR;
    resultMie *= betaM / 0.9;

    // return attenuations
    return exp(-(resultRayleigh + resultMie));
}

float DistanceToAtmosphereExit(vec3 start, vec3 dir)
{
    float b = 2.0 * dot(start, dir);
    float c = dot(start, start) - pow(planetRadius + atmosphereThickness, 2);
    float discriminant = b*b - 4.0*c;

    if(discriminant < 0.0)
        return 0.0;

    float t0 = (-b - sqrt(discriminant)) * 0.5;
    float t1 = (-b + sqrt(discriminant)) * 0.5;

    return max(t0, t1);
}