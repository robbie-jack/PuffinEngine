#include "ui/windows/ui_window_scene_hierarchy.h"

#include "scene/scene_graph_subsystem.h"
#include "node/node.h"

namespace puffin
{
	namespace ui
	{
		UIWindowSceneHierarchy::UIWindowSceneHierarchy(const std::shared_ptr<core::Engine>& engine): UIWindow(engine)
		{
		}

		void UIWindowSceneHierarchy::Draw(double deltaTime)
		{
			mWindowName = "Scene Hierarchy";

			const auto sceneGraphSubsystem = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				mEntityChanged = false;

				Begin(mWindowName);

				//List All Entities and their ID/Name
				ImVec2 listBoxSize = ImGui::GetWindowSize();
				listBoxSize.y -= 75.0f;

				ImGui::Text("Nodes"); ImGui::SameLine(ImGui::GetWindowWidth() * .5f); ImGui::Text("ID");

				if (ImGui::BeginListBox("##NodeList", listBoxSize))
				{
					constexpr ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_SpanAvailWidth;

					for (auto id : sceneGraphSubsystem->GetRootNodeIDs())
					{
						DrawNodeUI(id, baseFlags);
					}

					ImGui::EndListBox();
				}

				//if (ImGui::Button("Create Entity"))
				//{
				//	//ImGui::OpenPopup("Create Entity");
				//}

				//ImGui::SameLine();

				//if (ImGui::Button("Destroy Entity"))
				//{
				//	/*if (mSelectedEntity != gInvalidID)
				//	{
				//		mEnTTSubsystem->remove_entity(mSelectedEntity);

				//		mSelectedEntity = gInvalidID;

				//		mEntityChanged = true;
				//	}*/
				//}

				//if (ImGui::BeginPopup("Create Entity"))
				//{
				//	if (ImGui::Selectable("Default"))
				//	{
				//		const auto entity = mEnTTSubsystem->createEntity("Entity");

				//		const auto object = registry->get<SceneObjectComponent>(entity);
				//		mSelectedEntity = object.id;
				//		mEntityChanged = true;
				//	}

				//	if (ImGui::BeginMenu("2D"))
				//	{
				//		if (ImGui::Selectable("Mesh"))
				//		{
				//			const auto entity = mEnTTSubsystem->createEntity("Mesh");

				//			registry->emplace<TransformComponent2D>(entity);
				//			registry->emplace<rendering::MeshComponent>(entity);

				//			const auto object = registry->get<SceneObjectComponent>(entity);
				//			mSelectedEntity = object.id;
				//			mEntityChanged = true;
				//		}

				//		ImGui::EndMenu();
				//	}

				//	if (ImGui::BeginMenu("3D"))
				//	{
				//		if (ImGui::Selectable("Mesh"))
				//		{
				//			const auto entity = mEnTTSubsystem->createEntity("Mesh");

				//			//registry->emplace<TransformComponent3D>(entity);
				//			//registry->emplace<rendering::MeshComponent>(entity);

				//			const auto object = registry->get<SceneObjectComponent>(entity);
				//			mSelectedEntity = object.id;
				//			mEntityChanged = true;
				//		}

				//		ImGui::EndMenu();
				//	}

				//	ImGui::EndPopup();
				//}

				End();
			}
		}

		bool UIWindowSceneHierarchy::GetEntityChanged() const
		{
			return mEntityChanged;
		}

		void UIWindowSceneHierarchy::DrawNodeUI(UUID id, const ImGuiTreeNodeFlags& baseFlags)
		{
			ImGuiTreeNodeFlags tree_flags = baseFlags;

			auto scene_graph_subsystem = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();
			auto node = scene_graph_subsystem->GetNode(id);

			bool has_child = false;

			if (node->HasChildren())
				has_child = true;

				// Set Selected Flag if entity equals selectedEntity
			if (mSelectedEntity == id)
				tree_flags |= ImGuiTreeNodeFlags_Selected;

			// Display Entity as Leaf node if it doesn't have any children
			if (!has_child)
				tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

			const bool node_open = ImGui::TreeNodeEx(node->GetName().empty() ? "Empty" : node->GetName().c_str(), tree_flags);

			// Set Selected Entity when node is clicked
			if (ImGui::IsItemClicked())
			{
				mSelectedEntity = id;
				mEntityChanged = true;
			}

			// Display Entity ID on same line as name
			ImGui::SameLine(ImGui::GetWindowWidth() * .5f);
			ImGui::Text(std::to_string(id).c_str());

			if (has_child && node_open)
			{
				for (const auto childID : node->GetChildIDs())
				{
					DrawNodeUI(childID, baseFlags);
				}

				ImGui::TreePop();
			}
		}
	}
}
