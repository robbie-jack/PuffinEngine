#include "UIWindowMenu.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowMenu::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				/*windowSize.x = 400;
				windowSize.y = 600;
				ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
				ImGui::ShowDemoWindow();*/

				//Start Window
				if (!Begin("Performance"))
				{
					End();
				}
				else
				{
					// Display FPS
					fps_timer += dt;

					if (fps_timer >= 0.25f)
					{
						fps = 1 / dt;
						fps_timer = 0.0f;
					}

					ImGui::Text("FPS: %.1f", fps);
					ImGui::SliderFloat("Sensitivity", &InputManager->GetSensitivity(), 0.01f, 0.1f);

					return true;
				}
			}
		}
	}
}