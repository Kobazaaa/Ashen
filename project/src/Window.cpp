#include "Window.h"
#include <stdexcept>

Window::Window(int width, int height, const std::string& title)
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_pWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_pWindow)
        throw std::runtime_error("Failed to create GLFW window");
}

Window::~Window()
{
    if (m_pWindow)
        glfwDestroyWindow(m_pWindow);
    glfwTerminate();
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_pWindow);
}
void Window::PollEvents() const
{
    glfwPollEvents();
}
GLFWwindow* Window::GetGLFWwindow() const
{
	return m_pWindow;
}
