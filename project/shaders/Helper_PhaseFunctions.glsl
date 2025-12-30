// input
layout(set = 0, binding = 1) uniform PhaseParameters
{
    vec3 lightDir;				    // direction of the sunlight
    float g;						// constant that affects symmetry of the scattering
    float g2;						// g^2
};

// Different Phase Functions
float Phase_HenyeyGreenstein(float g, float g2, float cosine)
{
	float denom = pow(1 + g2 - 2*g*cosine, 1.5);
	float num = 1 - g2;
	return num / denom;
}

// Calculates the Mie phase function
float GetMiePhase(float cosine, float cosine2, float g, float g2)
{
	return Phase_HenyeyGreenstein(g, g2, cosine);
}

// Calculates the Rayleigh phase function
float GetRayleighPhase(float cosine2)
{
	return 0.75 + 0.75 * cosine2;
}
