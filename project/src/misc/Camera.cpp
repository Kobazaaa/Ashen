// -- Ashen Includes --
#include "Camera.h"

#include <algorithm>
#include "Timer.h"

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::Camera::Camera(Window* pWindow)
	: m_pWindow(pWindow)
{
	AspectRatio = m_pWindow->GetAspectRatio();
}

//--------------------------------------------------
//    Functionality
//--------------------------------------------------
void ashen::Camera::Update()
{
    auto move = Timer::GetDeltaSeconds() * Speed *
        (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT) ? 3.f :
            (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_CONTROL) ? 0.25f : 1.f));

	if (m_pWindow->IsKeyDown(GLFW_KEY_W)) Position += move * Forward;
	if (m_pWindow->IsKeyDown(GLFW_KEY_S)) Position -= move * Forward;
	if (m_pWindow->IsKeyDown(GLFW_KEY_A)) Position -= move * Right;
	if (m_pWindow->IsKeyDown(GLFW_KEY_D)) Position += move * Right;
	if (m_pWindow->IsKeyDown(GLFW_KEY_E)) Position += move * glm::vec3(0, 1, 0);
	if (m_pWindow->IsKeyDown(GLFW_KEY_Q)) Position -= move * glm::vec3(0, 1, 0);

    static bool firstMouse = true;
    static float lastX = 0.0;
    static float lastY = 0.0;

    if (m_pWindow->IsMouseDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        const auto mousePos = m_pWindow->GetCursorPos();
        if (firstMouse)
        {
            lastX = mousePos.x;
            lastY = mousePos.y;
            firstMouse = false;
        }

        float offsetX = mousePos.x - lastX;
        float offsetY = mousePos.y - lastY;
        lastX = mousePos.x;
        lastY = mousePos.y;

        offsetX *= Sensitivity;
        offsetY *= Sensitivity;

        Rotation.y += offsetX;
        Rotation.x += offsetY;

        Rotation.x = std::min(Rotation.x, 89.9f);
        Rotation.x = std::max(Rotation.x, -89.9f);

        GetViewMatrix();
    }
    else firstMouse = true;
}


//--------------------------------------------------
//    Accessors & Mutators
//--------------------------------------------------
glm::mat4 ashen::Camera::GetViewMatrix()
{
	const glm::mat4 T = glm::translate(glm::mat4(1.0f), Position);
	const glm::quat rotationQuaternion = glm::quat(glm::radians(Rotation));
	const glm::mat4 R = glm::mat4_cast(rotationQuaternion);

	auto mat = T * R;
	Right = glm::normalize(glm::vec3(mat[0]));
	Up = glm::normalize(glm::vec3(mat[1]));
	Forward = glm::normalize(glm::vec3(mat[2]));

	return glm::inverse(mat);
}
glm::mat4 ashen::Camera::GetProjectionMatrix() const
{
	auto mat = glm::perspectiveLH(glm::radians(Fov), AspectRatio, NearPlane, FarPlane);
	mat[1][1] *= -1;
	return mat;
}
