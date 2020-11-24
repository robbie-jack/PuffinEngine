#include "UIWindowSceneHierarchy.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "ReactPhysicsComponent.h"

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
						ECS::Entity entity = world->CreateEntity();
						windowProperties->SetEntity(entity);

						TransformComponent component;
						component.scale = Vector3(1.0f, 1.0f, 1.0f);
						world->AddComponent<TransformComponent>(entity, component);
					}

					ImGui::SameLine();

					if (ImGui::Button("Destroy Entity"))
					{
						windowProperties->SetEntity(ECS::INVALID_ENTITY);
						world->MarkToDelete(selectedEntity);
					}

					//List All Entities and their ID/Name
					ImVec2 listBoxSize = ImGui::GetWindowSize();
					listBoxSize.y -= 45.0f;

					ImGui::Text("Entities"); ImGui::SameLine(ImGui::GetWindowWidth() - 50.0f); ImGui::Text("ID");

					ImGui::ListBoxHeader("", listBoxSize); // Make ListBox fill Window

					if (world != nullptr)
					{
						ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow
							| ImGuiTreeNodeFlags_OpenOnDoubleClick
							| ImGuiTreeNodeFlags_SpanAvailWidth;

						for (ECS::Entity entity : world->GetActiveEntities())
						{
							ImGuiTreeNodeFlags tree_flags = base_flags;
							bool has_child = false;

							// Simple test to show leaf and branch nodes
							if (entity == 1 || entity == 2)
								has_child = true;

							// Set Selected Flag if entity equals selectedEntity
							if (selectedEntity == entity)
								tree_flags |= ImGuiTreeNodeFlags_Selected;

							// Display Entity as Leaf node if it doesn't have any children
							if (!has_child)
								tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

							bool node_open = ImGui::TreeNodeEx(world->GetEntityName(entity).c_str(), tree_flags);

							// Set Selected Entity when node is clicked
							if (ImGui::IsItemClicked())
							{
								selectedEntity = entity;
								windowProperties->SetEntity(entity);
							}

							// Display Entity ID on same line as name
							ImGui::SameLine(ImGui::GetWindowWidth() - 50.0f);
							ImGui::Text(std::to_string(entity).c_str());

							if (has_child && node_open)
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