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

	// -- Sky --
	struct SkyVS
	{
		float eT;
	};
	struct SkyFS
	{
		float eT;
	};

	// -- Ground --
	struct GroundVS
	{
		float eT;
	};
	struct GroundFS
	{
		float eT;
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