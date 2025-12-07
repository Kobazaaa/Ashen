#include <iostream>

#include "Renderer.h"
#include "VulkanContext.h"
#include "Window.h"

int main()
{
    std::unique_ptr<Window> pWindow = std::make_unique<Window>(800, 600, "Ashen");
    std::unique_ptr<VulkanContext> pContext = std::make_unique<VulkanContext>(pWindow.get());
    std::unique_ptr<Renderer> pRenderer = std::make_unique<Renderer>(pContext.get(), pWindow.get());

    while (!pWindow->ShouldClose())
    {
        pWindow->PollEvents();
        pRenderer->Render();
    }

    return 0;
}
