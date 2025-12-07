// -- Standard Library --
#include <stdexcept>

// -- Ashen Includes --
#include "Window.h"

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::Window::Window(int width, int height, const std::string& title)
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_pWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_pWindow)
        throw std::runtime_error("Failed to create GLFW window");

    glfwSetWindowUserPointer(m_pWindow, this);
    glfwSetFramebufferSizeCallback(m_pWindow, FrameBufferResizeCallback);
}
ashen::Window::~Window()
{
    if (m_pWindow)
        glfwDestroyWindow(m_pWindow);
    glfwTerminate();
}

//--------------------------------------------------
//    Functionality
//--------------------------------------------------
bool ashen::Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_pWindow);
}
void ashen::Window::PollEvents() const
{
    glfwPollEvents();
}
GLFWwindow* ashen::Window::GetGLFWwindow() const
{
	return m_pWindow;
}
float ashen::Window::GetAspectRatio() const
{
    int w;
    int h;
    glfwGetFramebufferSize(m_pWindow, &w, &h);
	return static_cast<float>(w) / static_cast<float>(h);
}
glm::uvec2 ashen::Window::GetFramebufferSize() const
{
    int w;
    int h;
    glfwGetFramebufferSize(m_pWindow, &w, &h);
    return { w, h };
}

//--------------------------------------------------
//    Keys
//--------------------------------------------------
bool ashen::Window::IsKeyDown(int key) const
{
	return glfwGetKey(m_pWindow, key) == GLFW_PRESS;
}
bool ashen::Window::IsMouseDown(int key) const
{
	return glfwGetMouseButton(m_pWindow, key) == GLFW_PRESS;
}
glm::vec2 ashen::Window::GetCursorPos() const
{
    double x;
    double y;
	glfwGetCursorPos(m_pWindow, &x, &y);

    return { x, y };
}

//--------------------------------------------------
//    Misc
//--------------------------------------------------
bool ashen::Window::IsOutdated() const
{
	return m_IsOutdated;
}
void ashen::Window::ResetOutdated()
{
    m_IsOutdated = false;
}
void ashen::Window::FrameBufferResizeCallback(GLFWwindow* window, int, int)
{
    Window* pWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    pWindow->m_IsOutdated = true;
}
