// input
layout(set = 0, binding = 1) uniform PhaseParameters
{
    vec3 lightDir;				    // direction of the sunlight
    float g;						// constant that affects symmetry of the scattering
    float g2;						// g^2
};

// Different Phase Functions
float Phase_CornetteShanks(float g, float g2, float cosine, float cosine2)
{
	return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + cosine2) / pow(1.0 + g2 - 2.0 * g * cosine, 1.5);
}


// Calculates the Mie phase function
float GetMiePhase(float cosine, float cosine2, float g, float g2)
{
    return Phase_CornetteShanks(g, g2, cosine, cosine2);
}

// Calculates the Rayleigh phase function
float GetRayleighPhase(float cosine2)
{
	return 0.75 + 0.75 * cosine2;
}
