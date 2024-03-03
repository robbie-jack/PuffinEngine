#include "UI/Editor/Windows/UIWindowSceneHierarchy.h"

#include "entt/entity/registry.hpp"
#include "Components/TransformComponent2D.h"
#include "Components/TransformComponent3D.h"
#include "Components/Rendering/MeshComponent.h"
#include "ECS/EnTTSubsystem.h"

namespace puffin
{
	namespace ui
	{
		void UIWindowSceneHierarchy::draw(double dt)
		{
			mWindowName = "Scene Hierarchy";

			if (!mEnTTSubsystem)
			{
				mEnTTSubsystem = mEngine->getSystem<ecs::EnTTSubsystem>();
			}

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				mEntityChanged = false;

				begin(mWindowName);

				//List All Entities and their ID/Name
				ImVec2 listBoxSize = ImGui::GetWindowSize();
				listBoxSize.y -= 75.0f;

				ImGui::Text("Entities"); ImGui::SameLine(ImGui::GetWindowWidth() * .5f); ImGui::Text("ID");

				const auto registry = mEnTTSubsystem->registry();

				if (ImGui::BeginListBox("##EntityList", listBoxSize))
				{
					constexpr ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_SpanAvailWidth;

					const auto entityView = registry->view<const SceneObjectComponent>();

					for (auto [entity, object] : entityView.each())
					{
						ImGuiTreeNodeFlags treeFlags = baseFlags;
						constexpr bool hasChild = false;

						// Simple test to show leaf and branch nodes
						/*if (entity == 1 || entity == 2)
							has_child = true;*/

						// Set Selected Flag if entity equals selectedEntity
						if (mSelectedEntity == object.id)
							treeFlags |= ImGuiTreeNodeFlags_Selected;

						// Display Entity as Leaf node if it doesn't have any children
						if constexpr (!hasChild)
							treeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

						bool nodeOpen = false;

						nodeOpen = ImGui::TreeNodeEx(object.name.empty() ? "Empty" : object.name.c_str(), treeFlags);

						// Set Selected Entity when node is clicked
						if (ImGui::IsItemClicked())
						{
							mSelectedEntity = object.id;
							mEntityChanged = true;
						}

						// Display Entity ID on same line as name
						ImGui::SameLine(ImGui::GetWindowWidth() * .5f);
						ImGui::Text(std::to_string(object.id).c_str());

						if constexpr (hasChild && nodeOpen)
						{
							ImGui::Text("No Child Entities");
							ImGui::TreePop();
						}
					}

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
						mEnTTSubsystem->destroyEntity(mSelectedEntity);

						mSelectedEntity = gInvalidID;

						mEntityChanged = true;
					}
				}

				if (ImGui::BeginPopup("Create Entity"))
				{
					if (ImGui::Selectable("Default"))
					{
						const auto entity = mEnTTSubsystem->createEntity("Entity");

						const auto object = registry->get<SceneObjectComponent>(entity);
						mSelectedEntity = object.id;
						mEntityChanged = true;
					}

					if (ImGui::BeginMenu("2D"))
					{
						if (ImGui::Selectable("Mesh"))
						{
							const auto entity = mEnTTSubsystem->createEntity("Mesh");

							registry->emplace<TransformComponent2D>(entity);
							registry->emplace<rendering::MeshComponent>(entity);

							const auto object = registry->get<SceneObjectComponent>(entity);
							mSelectedEntity = object.id;
							mEntityChanged = true;
						}

						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("3D"))
					{
						if (ImGui::Selectable("Mesh"))
						{
							const auto entity = mEnTTSubsystem->createEntity("Mesh");

							//registry->emplace<TransformComponent3D>(entity);
							//registry->emplace<rendering::MeshComponent>(entity);

							const auto object = registry->get<SceneObjectComponent>(entity);
							mSelectedEntity = object.id;
							mEntityChanged = true;
						}

						ImGui::EndMenu();
					}

					ImGui::EndPopup();
				}

				end();
			}
		}
	}
}
