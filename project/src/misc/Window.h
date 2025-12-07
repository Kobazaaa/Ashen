#ifndef ASHEN_WINDOW_H
#define ASHEN_WINDOW_H

#include <GLFW/glfw3.h>
#include <string>
#include "glm/vec2.hpp"

namespace ashen
{
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~    Window
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    class Window final
    {
    public:
        //--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
        Window(int width, int height, const std::string& title);
        ~Window();
        Window(const Window& other) = delete;
        Window(Window&& other) = delete;
        Window& operator=(const Window& other) = delete;
        Window& operator=(Window&& other) = delete;

        //--------------------------------------------------
		//    Functionality
		//--------------------------------------------------
        bool ShouldClose() const;
        void PollEvents() const;
        GLFWwindow* GetGLFWwindow() const;
        float GetAspectRatio() const;
        glm::uvec2 GetFramebufferSize() const;

        bool IsKeyDown(int key) const;
        bool IsMouseDown(int key) const;
        glm::vec2 GetCursorPos() const;

        bool IsOutdated() const;
        void ResetOutdated();
        static void FrameBufferResizeCallback(GLFWwindow* window, int width, int height);

    private:
        GLFWwindow* m_pWindow = nullptr;
        bool m_IsOutdated = false;
    };

}

#endif // ASHEN_WINDOW_H