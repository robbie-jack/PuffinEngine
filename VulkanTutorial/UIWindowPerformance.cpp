#include "UIWindowPerformance.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowPerformance::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				windowName = "Performance";
				if (!Begin(windowName))
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

					ImGui::Text("Framerate: %d", (int)fps);
				}
			}

			return true;
		}
	}
}