#include "UIWindowMenu.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowMenu::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);

				flags = ImGuiWindowFlags_MenuBar;

				if (!Begin("Puffin Engine"))
				{
					End();
				}
				else
				{
					// Menu Bar
					if (ImGui::BeginMenuBar())
					{
						if (ImGui::BeginMenu("Menu"))
						{
							if (ImGui::MenuItem("Quit", "Alt+F4"))
							{
								return false;
							}

							ImGui::EndMenu();
						}

						ImGui::EndMenuBar();
					}

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