#include "UIWindowSceneHierarchy.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowSceneHierarchy::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			windowName = "Scene Hierarchy";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

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

					ImGui::SameLine();

					if (ImGui::Button("Destroy Entity"))
					{
						windowProperties->SetEntity(nullptr);
						entitySystem->DestroyEntity(selectedID);
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
							selectedID = entityID;
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