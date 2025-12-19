// -- Standard Library --
#include <iostream>

// -- Ashen Includes --
#include "Renderer.h"
#include "Timer.h"
#include "Window.h"
#include "ConsoleTextSettings.h"
using namespace ashen;

int main()
{
	std::unique_ptr<Window> pWindow = std::make_unique<Window>(800, 600, "Ashen");
    std::unique_ptr<Renderer> pRenderer = std::make_unique<Renderer>(pWindow.get());

    Timer::Start();
    while (!pWindow->ShouldClose())
    {
        Timer::Update();
        pWindow->PollEvents();
        pRenderer->Update();
        pRenderer->Render();
    }

    return 0;
}
