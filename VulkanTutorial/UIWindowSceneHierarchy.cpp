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
						windowProperties->SetEntity(entityManager->CreateEntity());
					}

					ImGui::SameLine();

					if (ImGui::Button("Destroy Entity"))
					{
						windowProperties->SetEntity(ECS::INVALID_ENTITY);
						entityManager->DestroyEntity(selectedEntity);
					}

					//List All Entities and their ID/Name
					ImVec2 listBoxSize = ImGui::GetWindowSize();
					listBoxSize.y -= 45.0f;

					ImGui::ListBoxHeader("", listBoxSize); // Make ListBox fill Window

					if (entityManager != nullptr)
					{
						for (ECS::Entity entity : entityManager->GetActiveEntities())
						{
							std::string entity_string = "Entity: " + std::to_string(entity);
							if (ImGui::Selectable(entity_string.c_str(), false))
							{
								selectedEntity = entity;
								windowProperties->SetEntity(entity);
							}
						}
					}

					ImGui::ListBoxFooter();
				}
			}

			return true;
		}
	}
}