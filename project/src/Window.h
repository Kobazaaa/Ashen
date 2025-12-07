#ifndef ASHEN_WINDOW_H
#define ASHEN_WINDOW_H

#include <GLFW/glfw3.h>
#include <string>

class Window final
{
public:
    Window(int width, int height, const std::string& title);
    ~Window();
    Window(const Window& other) = delete;
    Window(Window&& other) = delete;
    Window& operator=(const Window& other) = delete;
    Window& operator=(Window&& other) = delete;

    bool ShouldClose() const;
    void PollEvents() const;
    GLFWwindow* GetGLFWwindow() const;

private:
    GLFWwindow* m_pWindow = nullptr;
};

#endif // ASHEN_WINDOW_H