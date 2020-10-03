#include "UIWindowEntityProperties.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowEntityProperties::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				windowName = "Entity Properties";
				if (!Begin(windowName))
				{
					End();
				}
				else
				{

				}
			}

			return true;
		}
	}
}