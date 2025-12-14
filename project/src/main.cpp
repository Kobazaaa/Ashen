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
    float elapsedTime = 0.f;
    while (!pWindow->ShouldClose())
    {
        Timer::Update();
        pWindow->PollEvents();
        pRenderer->Update();
        pRenderer->Render();

        elapsedTime += Timer::GetDeltaSeconds();
        if (elapsedTime >= 1.0f)
        {
            elapsedTime = 0.f;
            std::cout << DARK_YELLOW_TXT << "dFPS: " << 1.f / Timer::GetDeltaSeconds() << RESET_TXT << "\n";
        }
    }

    return 0;
}
