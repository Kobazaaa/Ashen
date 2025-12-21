#ifndef ASHEN_CAMERA_H
#define ASHEN_CAMERA_H

// -- Math Includes --
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

// -- Ashen Includes --
#include "Window.h"

namespace ashen
{
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //? ~~    Camera
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    class Camera
    {
    public:
        //--------------------------------------------------
        //    Constructor & Destructor
        //--------------------------------------------------
        explicit Camera(Window* pWindow);
        ~Camera() = default;

		Camera(const Camera& other)					= default;
		Camera(Camera&& other) noexcept				= default;
		Camera& operator=(const Camera& other)		= default;
		Camera& operator=(Camera&& other) noexcept	= default;

		//--------------------------------------------------
		//    Functionality
		//--------------------------------------------------
        void Update();

        float Speed{ 1.f };
        float Sensitivity{ 0.1f };
        float Fov{60.f};
        float AspectRatio{};
        float NearPlane{0.001f};
        float FarPlane{10000.f};

        glm::vec3 Position{};
        glm::vec3 Rotation{};
        glm::vec3 Forward{};
        glm::vec3 Right{};
        glm::vec3 Up{};
        //--------------------------------------------------
		//    Accessors & Mutators
		//--------------------------------------------------
        glm::mat4 GetViewMatrix();
        glm::mat4 GetProjectionMatrix() const;

    private:
        Window* m_pWindow{} ;
    };
}

#endif // ASHEN_CAMERA_H
