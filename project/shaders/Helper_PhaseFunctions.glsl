// The following functions are provided by Sean O'Neil from the demo code from GPU Gems 2 Chapter 16: Accurate Atmospheric Scattering
// https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering

// Calculates the Mie phase function
float GetMiePhase(float cosine, float cosine2, float g, float g2)
{
	return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + cosine2) / pow(1.0 + g2 - 2.0 * g * cosine, 1.5);
}

// Calculates the Rayleigh phase function
float GetRayleighPhase(float cos2)
{
	return 0.75 + 0.75 * cos2;
}
