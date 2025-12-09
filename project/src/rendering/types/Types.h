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
}

#endif // ASHEN_TYPES_H