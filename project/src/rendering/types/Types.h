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

		glm::vec3 lightDir;				// direction of the sunlight
		float cameraHeight2;			// cameraHeight^2

		glm::vec3 invWaveLength;		// 1 / (wavelength^4) for RGB
		float sampleCount;		        // nr of samples along the ray

		float outerRadius;				// outer atmosphere radius
		float outerRadius2;				// outerRadius^2
		float innerRadius;				// inner planetary radius
		float innerRadius2;				// innerRadius^2

		float scale;					// 1 / (outerRadius - innerRadius)
		float scaleDepth;				// scale depth (the altitude at which the average atmospheric density is found)

		float koe;		                // Ozone Extinction Coefficient
		float krESun;					// Kr * ESun
		float kmESun;					// Km * ESun
		float kr4PI;					// Kr * 4 * PI
		float km4PI;					// Km * 4 * PI
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
		glm::vec3 cameraPos;			// current camera pos
		float cameraHeight;				// current camera height

		glm::vec3 lightDir;				// direction of the sunlight
		float cameraHeight2;			// cameraHeight^2

		glm::vec3 invWaveLength;		// 1 / (wavelength^4) for RGB
		float sampleCount;		        // nr of samples along the ray

		float outerRadius;				// outer atmosphere radius
		float outerRadius2;				// outerRadius^2
		float innerRadius;				// inner planetary radius
		float innerRadius2;				// innerRadius^2

		float scale;					// 1 / (outerRadius - innerRadius)
		float scaleDepth;				// scale depth (the altitude at which the average atmospheric density is found)

		float koe;		                // Ozone Extinction Coefficient
		float krESun;					// Kr * ESun
		float kmESun;					// Km * ESun
		float kr4PI;					// Kr * 4 * PI
		float km4PI;					// Km * 4 * PI
	};
	struct GroundFS
	{
		float n;
	};

	// -- Space --
	struct SpaceVS
	{
		float eT;
	};
	struct SpaceFS
	{
		float eT;
	};
}

#endif // ASHEN_TYPES_H