#include "UIWindowEntityProperties.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowEntityProperties::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			windowName = "Entity Properties";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				if (!Begin(windowName))
				{
					End();
				}
				else
				{
					std::string entityIDString;

					if (entity != nullptr)
					{
						entityIDString = " Entity ID: " + std::to_string(entity->GetID());
					}
					else
					{
						entityIDString = " No Entity Selected";
					}

					ImGui::Text(entityIDString.c_str());
				}
			}

			return true;
		}
	}
}