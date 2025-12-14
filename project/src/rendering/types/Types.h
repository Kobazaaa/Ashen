#ifndef ASHEN_TYPES_H
#define ASHEN_TYPES_H

// -- Math Includes --
#include <glm/glm.hpp>

namespace ashen
{
	struct TriangleShaderPCV
	{
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct SkyFromSpaceVS
	{
		float eT;
	};
	struct SkyFromSpaceFS
	{
		
	};

	struct SkyFromAtmosphereVS
	{
		
	};
	struct SkyFromAtmosphereFS
	{
		
	};

	struct GroundFromSpaceVS
	{
		
	};
	struct GroundFromSpaceFS
	{
		
	};

	struct GroundFromAtmosphereVS
	{
		
	};
	struct GroundFromAtmosphereFS
	{
		
	};

	struct SpaceFromSpaceVS
	{
		
	};
	struct SpaceFromSpaceFS
	{
		
	};

	struct SpaceFromAtmosphereVS
	{
		
	};
	struct SpaceFromAtmosphereFS
	{
		
	};
}

#endif // ASHEN_TYPES_H