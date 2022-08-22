#include "UIWindowSceneHierarchy.h"

#include "Components/TransformComponent.h"

#include "ECS/ECS.h"
#include "ECS/Entity.h"

namespace Puffin
{
	namespace UI
	{
		void UIWindowSceneHierarchy::Draw(float dt)
		{
			windowName = "Scene Hierarchy";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				entityChanged = false;

				Begin(windowName);

				//List All Entities and their ID/Name
				ImVec2 listBoxSize = ImGui::GetWindowSize();
				listBoxSize.y -= 75.0f;

				ImGui::Text("Entities"); ImGui::SameLine(ImGui::GetWindowWidth() - 50.0f); ImGui::Text("ID");

				ImGui::ListBoxHeader("", listBoxSize); // Make ListBox fill Window

				auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
				if (ecsWorld)
				{
					ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_SpanAvailWidth;

					for (ECS::EntityID entity : ecsWorld->GetActiveEntities())
					{
						ImGuiTreeNodeFlags tree_flags = base_flags;
						bool has_child = false;

						// Simple test to show leaf and branch nodes
						/*if (entity == 1 || entity == 2)
							has_child = true;*/

						// Set Selected Flag if entity equals selectedEntity
						if (selectedEntity == entity)
							tree_flags |= ImGuiTreeNodeFlags_Selected;

						// Display Entity as Leaf node if it doesn't have any children
						if (!has_child)
							tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

						bool node_open = ImGui::TreeNodeEx(ecsWorld->GetEntityName(entity).c_str(), tree_flags);

						// Set Selected Entity when node is clicked
						if (ImGui::IsItemClicked())
						{
							selectedEntity = entity;
							entityChanged = true;
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

				if (ImGui::Button("Create Entity"))
				{
					ImGui::OpenPopup("Create Entity");
				}

				ImGui::SameLine();

				if (ImGui::Button("Destroy Entity"))
				{
					ecsWorld->SetEntityFlag<FlagDeleted>(selectedEntity, true);

					selectedEntity = ECS::INVALID_ENTITY;
					entityChanged = true;
				}

				if (ImGui::BeginPopup("Create Entity"))
				{
					if (ImGui::Selectable("Empty"))
					{
						const auto entity = ECS::CreateEntity(ecsWorld);

						entity->AddComponent<TransformComponent>();

						selectedEntity = entity->ID();
						entityChanged = true;
					}

					ImGui::EndPopup();
				}

				End();
			}
		}
	}
}