// The following functions are provided by Sean O'Neil from the demo code from GPU Gems 2 Chapter 16: Accurate Atmospheric Scattering
// https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering

// Returns the near intersection point of a line and a sphere
float GetNearIntersection(vec3 pos, vec3 ray, float distance2, float radius2)
{
	float B = 2.0 * dot(pos, ray);
	float C = distance2 - radius2;
	float det = max(0.0, B * B - 4.0 * C);
	return 0.5 * (-B - sqrt(det));
}

// Returns the far intersection point of a line and a sphere
float GetFarIntersection(vec3 pos, vec3 ray, float distance2, float radius2)
{
	float B = 2.0 * dot(pos, ray);
	float C = distance2 - radius2;
	float det = max(0.0, B * B - 4.0 * C);
	return 0.5 * (-B + sqrt(det));
}
