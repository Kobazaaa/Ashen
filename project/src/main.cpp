#include <iostream>
#include "VulkanContext.h"
#include "Window.h"

int main()
{
    std::unique_ptr<Window> pWindow = std::make_unique<Window>(800, 600, "Ashen");
    std::unique_ptr<VulkanContext> pContext = std::make_unique<VulkanContext>(pWindow.get());
    pContext;

    while (!pWindow->ShouldClose())
    {
        pWindow->PollEvents();

    }

    return 0;
}
