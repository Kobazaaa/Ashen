// The following functions are provided by Sean O'Neil from the demo code from GPU Gems 2 Chapter 16: Accurate Atmospheric Scattering
// https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering

// The scale equation calculated by Vernier's Graphical Analysis
float Scale(float cosine, float scaleDepth)
{
	float x = 1.0 - cosine;
	return scaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}
