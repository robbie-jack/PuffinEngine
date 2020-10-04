#include "UIWindowEntities.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowEntities::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				windowName = "Entities";
				if (!Begin(windowName))
				{
					End();
				}
				else
				{
					if (ImGui::Button("Create Entity"))
					{
						windowProperties->SetEntity(entitySystem->GetEntity(entitySystem->CreateEntity()));
					}

					// List All Entities and their ID/Name
					ImVec2 listBoxSize = ImGui::GetWindowSize();
					listBoxSize.y -= 45.0f;

					ImGui::ListBoxHeader("", listBoxSize); // Make ListBox fill Window

					for (uint32_t entityID : entitySystem->GetEntityIDVector())
					{
						std::string entity_string = "ID: " + std::to_string(entityID);
						if (ImGui::Selectable(entity_string.c_str(), false))
						{
							windowProperties->SetEntity(entitySystem->GetEntity(entityID));
						}
					}

					ImGui::ListBoxFooter();
				}
			}

			return true;
		}
	}
}