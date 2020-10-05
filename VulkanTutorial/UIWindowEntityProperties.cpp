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
					if (entity != nullptr)
					{
						ImGui::Text(" Entity ID: %s", std::to_string(entity->GetID()));
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						ImGui::Text(" List of Components Here");

						std::vector<BaseComponent*> entityComponents = entity->GetComponents();

						for (int i = 0; i < entityComponents.size(); i++)
						{
							//ImGui::Text(" %s", entityComponents[i]->GetName().c_str());
						}
					}
					else
					{
						ImGui::Text(" No Entity Selected");
					}

					
				}
			}

			return true;
		}
	}
}