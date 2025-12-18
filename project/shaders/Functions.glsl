// The scale equation calculated by Vernier's Graphical Analysis
float Scale(float cosine, float scaleDepth)
{
	float x = 1.0 - cosine;
	return scaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

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
