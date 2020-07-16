#include "UIManager.h"

namespace Puffin
{
	namespace UI
	{
		UIManager::UIManager()
		{
			running = true;
			fps = 60.0f;
			fps_timer = 0.0f;
		}

		UIManager::~UIManager()
		{

		}

		bool UIManager::DrawUI(float dt, Input::InputManager* InputManager)
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::SetNextWindowSize(ImVec2(800, 1000), ImGuiCond_FirstUseEver);

			bool* p_open = NULL;

			// Draw UI Windows
			if (windows.size() > 0)
			{
				for (int i = 0; i < windows.size(); i++)
				{
					if (!windows[i]->Draw(dt, InputManager))
					{
						running = false;
					}
				}
			}

			ImGui::Render();

			return running;
		}

		void UIManager::AddWindow(UIWindow* window)
		{
			windows.push_back(window);
		}
	}
}