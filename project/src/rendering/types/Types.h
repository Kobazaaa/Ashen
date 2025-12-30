#ifndef ASHEN_TYPES_H
#define ASHEN_TYPES_H

// -- Math Includes --
#include <glm/glm.hpp>

namespace ashen
{
	// -- Camera --
	struct CameraMatricesPC
	{
		glm::mat4 view;
		glm::mat4 proj;
	};
	struct Exposure
	{
		float exposure;
	};

	// -- Sky --
	struct SkyVS
	{
		glm::vec3 cameraPos;			// current camera pos
		float cameraHeight;				// current camera height

		glm::vec3 lightDir;				// direction of to sunlight
		float sampleCount;		        // nr of samples along the ray

		glm::vec3 betaR;				// rayleigh coefficient
		float atmosphereThickness;      // thickness of the atmosphere

		glm::vec3 betaM;				// mie coefficient
		float planetRadius;				// planetary radius

		glm::vec3 betaO;				// ozone coefficient
		float rayleighScaleHeight;      // scale height rayleigh (the altitude at which the average atmospheric density is found)
		float mieScaleHeigh;            // scale height mie (the altitude at which the average atmospheric density is found)
		float sunIntensity;             // intensity of the sun

		float renderRadius;				// the radius of the planet at which it is renderer
		float renderThickness;			// the thickness of the atmosphere at which it is renderer
	};
	struct SkyFS
	{
		glm::vec3 lightDir;				// direction of the sunlight
		float g;						// constant that affects symmetry of the scattering
		float g2;						// g^2
	};

	// -- Ground --
	struct GroundVS
	{
		glm::vec3 lightDir;				// direction of to sunlight
	};
}

#endif // ASHEN_TYPES_H