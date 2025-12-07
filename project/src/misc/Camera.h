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

        float Speed{ 100.f };
        float Sensitivity{ 0.1f };
        float Fov{60.f};
        float AspectRatio{};
        float NearPlane{0.01f};
        float FarPlane{10000.f};

		//--------------------------------------------------
		//    Accessors & Mutators
		//--------------------------------------------------
        glm::mat4 GetViewMatrix();
        glm::mat4 GetProjectionMatrix() const;

    private:
        bool IsKeyDown(int key);

        Window* m_pWindow{} ;

        glm::vec3 m_Position{};
        glm::vec3 m_Rotation{};
        glm::vec3 m_Forward{};
        glm::vec3 m_Right{};
        glm::vec3 m_Up{};

    };
}

#endif // ASHEN_CAMERA_H
