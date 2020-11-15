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
						windowProperties->SetEntity(world->CreateEntity());
					}

					ImGui::SameLine();

					if (ImGui::Button("Destroy Entity"))
					{
						windowProperties->SetEntity(ECS::INVALID_ENTITY);
						world->DestroyEntity(selectedEntity);
					}

					//List All Entities and their ID/Name
					ImVec2 listBoxSize = ImGui::GetWindowSize();
					listBoxSize.y -= 45.0f;

					ImGui::ListBoxHeader("", listBoxSize); // Make ListBox fill Window

					if (world != nullptr)
					{
						ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow
							| ImGuiTreeNodeFlags_OpenOnDoubleClick
							| ImGuiTreeNodeFlags_SpanAvailWidth;

						for (ECS::Entity entity : world->GetActiveEntities())
						{
							std::string entity_string = "Entity: " + std::to_string(entity);

							ImGuiTreeNodeFlags tree_flags = base_flags;

							if (selectedEntity == entity)
								tree_flags |= ImGuiTreeNodeFlags_Selected;

							bool node_open = ImGui::TreeNodeEx(entity_string.c_str(), tree_flags);
							if (ImGui::IsItemClicked())
							{
								selectedEntity = entity;
								windowProperties->SetEntity(entity);
							}

							if (node_open)
							{
								ImGui::Text("No Child Entities");
								ImGui::TreePop();
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