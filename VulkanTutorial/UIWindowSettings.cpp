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
					ImGui::SliderFloat("Sensitivity", &InputManager->GetSensitivity(), 0.01f, 0.1f);
				}
			}

			return true;
		}
	}
}