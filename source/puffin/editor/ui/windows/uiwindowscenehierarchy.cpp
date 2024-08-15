#include "puffin/editor/ui/windows/uiwindowscenehierarchy.h"

#include "puffin/scene/scenegraph.h"
#include "puffin/nodes/node.h"

namespace puffin
{
	namespace ui
	{
		void UIWindowSceneHierarchy::draw(double dt)
		{
			mWindowName = "Scene Hierarchy";

			auto scene_graph_subsystem = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				m_entity_changed = false;

				begin(mWindowName);

				//List All Entities and their ID/Name
				ImVec2 listBoxSize = ImGui::GetWindowSize();
				listBoxSize.y -= 75.0f;

				ImGui::Text("Nodes"); ImGui::SameLine(ImGui::GetWindowWidth() * .5f); ImGui::Text("ID");

				if (ImGui::BeginListBox("##NodeList", listBoxSize))
				{
					constexpr ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_SpanAvailWidth;

					for (auto id : scene_graph_subsystem->get_root_node_ids())
					{
						draw_node_ui(id, baseFlags);
					}

					ImGui::EndListBox();
				}

				

				if (ImGui::Button("Create Entity"))
				{
					//ImGui::OpenPopup("Create Entity");
				}

				ImGui::SameLine();

				if (ImGui::Button("Destroy Entity"))
				{
					/*if (mSelectedEntity != gInvalidID)
					{
						mEnTTSubsystem->remove_entity(mSelectedEntity);

						mSelectedEntity = gInvalidID;

						mEntityChanged = true;
					}*/
				}

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

				end();
			}
		}

		void UIWindowSceneHierarchy::draw_node_ui(PuffinID id, const ImGuiTreeNodeFlags& base_flags)
		{
			ImGuiTreeNodeFlags tree_flags = base_flags;

			auto scene_graph_subsystem = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();
			auto node = scene_graph_subsystem->get_node_ptr(id);

			bool has_child = false;

			std::vector<PuffinID> child_ids;
			node->get_child_ids(child_ids);

			if (!child_ids.empty())
				has_child = true;

				// Set Selected Flag if entity equals selectedEntity
			if (mSelectedEntity == id)
				tree_flags |= ImGuiTreeNodeFlags_Selected;

			// Display Entity as Leaf node if it doesn't have any children
			if (!has_child)
				tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

			const bool node_open = ImGui::TreeNodeEx(node->name().empty() ? "Empty" : node->name().c_str(), tree_flags);

			// Set Selected Entity when node is clicked
			if (ImGui::IsItemClicked())
			{
				mSelectedEntity = id;
				m_entity_changed = true;
			}

			// Display Entity ID on same line as name
			ImGui::SameLine(ImGui::GetWindowWidth() * .5f);
			ImGui::Text(std::to_string(id).c_str());

			if (has_child && node_open)
			{
				for (auto id : child_ids)
				{
					draw_node_ui(id, base_flags);
				}

				ImGui::TreePop();
			}
		}
	}
}
