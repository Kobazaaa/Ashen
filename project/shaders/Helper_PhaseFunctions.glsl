// Constants
const float PI = 3.14159265359;
const float INV_PI = 0.31830988618;
const float INV_4PI = 0.07957747154;

// input
layout(set = 0, binding = 1) uniform Parameters
{
    vec3 lightDir;				    // direction of the sunlight
    float g;						// constant that affects symmetry of the scattering
    float g2;						// g^2
};

// Calculates the Mie phase function
float GetMiePhase(float cosine, float cosine2, float g, float g2)
{
	return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + cosine2) / pow(1.0 + g2 - 2.0 * g * cosine, 1.5);
}

// Calculates the Rayleigh phase function
float GetRayleighPhase(float cosine2)
{
	return 0.75 + 0.75 * cosine2;
}


// Different Phase Functions
float PhaseMie_HenyeyGreenstein(float g, float g2, float cosine)
{
	float denom = pow(1 + g2 + 2*g*cosine, 1.5);
	float num = 1 - g2;
	return INV_4PI * num / denom;
}
float PhaseRayleigh_HenyeyGreenstein(float g, float g2, float cosine)
{
	// This is the same as the Henyey-Greenstein Phase Function for Mie scattering but with g = 0
	return INV_4PI;
}

float PhaseMie_CornetteShanks(float g, float g2, float cosine, float cosine2)
{
	return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + cosine2) / pow(1.0 + g2 - 2.0 * g * cosine, 1.5);
}
float PhaseRayleigh_CornetteShanks(float cosine2)
{
	// This is the same as the Corenette-Shanks Phase Function for Mie scattering but with g = 0
	return 0.75 + 0.75 * cosine2;
}