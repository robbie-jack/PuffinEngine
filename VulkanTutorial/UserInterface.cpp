#include "UserInterface.h"

namespace Puffin
{
	namespace UI
	{
		UserInterface::UserInterface()
		{
			running = true;
			fps = 60.0f;
			fps_timer = 0.0f;
		}

		UserInterface::~UserInterface()
		{

		}

		bool UserInterface::DrawUI(float dt, Input::InputManager* InputManager)
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::SetNextWindowSize(ImVec2(800, 1000), ImGuiCond_FirstUseEver);

			bool* p_open = NULL;

			// Main body of the window starts here.
			ImGui::Begin("Puffin Engine", p_open, ImGuiWindowFlags_MenuBar);

			// Menu Bar
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Menu"))
				{
					if (ImGui::MenuItem("Quit", "Alt+F4"))
					{
						running = false;
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

			ImGui::End();

			ImGui::Render();

			return running;
		}
	}
}