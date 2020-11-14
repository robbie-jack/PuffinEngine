#include "UIWindowSettings.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowSettings::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			windowName = "Settings";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				if (!Begin(windowName))
				{
					End();
				}
				else
				{
					float tempfov = fov;

					ImGui::SliderFloat("Sensitivity", &InputManager->GetSensitivity(), 0.01f, 0.1f);
					ImGui::SliderFloat("Field of View", &fov, 30.0f, 120.0f, "%f");

					// If FOV has been changed this frame, update camera fov
					if (fov != tempfov)
					{
						camera->fov = fov;
					}
				}
			}

			return true;
		}
	}
}