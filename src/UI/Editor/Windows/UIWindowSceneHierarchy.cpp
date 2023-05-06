#include "UI/Editor/Windows/UIWindowSceneHierarchy.h"

#include "entt/entity/registry.hpp"
#include "Components/TransformComponent.h"
#include "ECS/EnTTSubsystem.h"

namespace puffin
{
	namespace ui
	{
		void UIWindowSceneHierarchy::draw(double dt)
		{
			mWindowName = "Scene Hierarchy";

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				mEntityChanged = false;

				begin(mWindowName);

				//List All Entities and their ID/Name
				ImVec2 listBoxSize = ImGui::GetWindowSize();
				listBoxSize.y -= 75.0f;

				ImGui::Text("Entities"); ImGui::SameLine(ImGui::GetWindowWidth() * .5f); ImGui::Text("ID");

				//const auto ecsWorld = m_engine->getSubsystem<ECS::World>();
				const auto enttSubsystem = mEngine->getSubsystem<ecs::EnTTSubsystem>();
				const auto registry = enttSubsystem->registry();

				if (ImGui::BeginListBox("##EntityList", listBoxSize))
				{
					ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_SpanAvailWidth;

					//for (ECS::EntityID entity : ecsWorld->GetActiveEntities())
					//{
					//	ImGuiTreeNodeFlags tree_flags = base_flags;
					//	bool has_child = false;

					//	// Simple test to show leaf and branch nodes
					//	/*if (entity == 1 || entity == 2)
					//		has_child = true;*/

					//	// Set Selected Flag if entity equals selectedEntity
					//	if (selectedEntity == entity)
					//		tree_flags |= ImGuiTreeNodeFlags_Selected;

					//	// Display Entity as Leaf node if it doesn't have any children
					//	if (!has_child)
					//		tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

					//	bool node_open = ImGui::TreeNodeEx(ecsWorld->GetEntityName(entity).c_str(), tree_flags);

					//	// Set Selected Entity when node is clicked
					//	if (ImGui::IsItemClicked())
					//	{
					//		selectedEntity = entity;
					//		entityChanged = true;
					//	}

					//	// Display Entity ID on same line as name
					//	ImGui::SameLine(ImGui::GetWindowWidth() * .5f);
					//	ImGui::Text(std::to_string(entity).c_str());

					//	if (has_child && node_open)
					//	{
					//		ImGui::Text("No Child Entities");
					//		ImGui::TreePop();
					//	}

					//}

					ImGui::EndListBox();
				}

				

				if (ImGui::Button("Create Entity"))
				{
					ImGui::OpenPopup("Create Entity");
				}

				ImGui::SameLine();

				if (ImGui::Button("Destroy Entity"))
				{
					if (mSelectedEntity != gInvalidID)
					{
						registry->destroy(enttSubsystem->getEntity(mSelectedEntity));

						mSelectedEntity = gInvalidID;

						mEntityChanged = true;
					}
				}

				if (ImGui::BeginPopup("Create Entity"))
				{
					if (ImGui::Selectable("Empty"))
					{
						const auto entity = enttSubsystem->createEntity("Entity");
						const auto object = registry->get<SceneObjectComponent>(entity);

						mSelectedEntity = object.id;

						mEntityChanged = true;
					}

					ImGui::EndPopup();
				}

				end();
			}
		}
	}
}
