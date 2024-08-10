#pragma once

#include <memory>

#include "puffin/ui/editor/windows/ui_window.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/nodes/node.h"

namespace ImGui
{
	class FileBrowser;
}

namespace puffin
{
	namespace scene
	{
		class SceneGraphSubsystem;
	}

	namespace scripting
	{
		struct AngelScriptComponent;
	}

	namespace rendering
	{
		struct LightComponent;
		struct MeshComponent;
		struct ShadowCasterComponent;
	}

	namespace procedural
	{
		struct PlaneComponent;
	}

	namespace physics
	{
		struct BoxComponent2D;
		struct CircleComponent2D;
		struct RigidbodyComponent2D;
	}

	struct TransformComponent2D;
	struct TransformComponent3D;
}

namespace puffin::ui
{
	class UIWindowNodeEditor : public UIWindow
	{
		class IComponentHandler
		{
		public:

			IComponentHandler(const std::string& name) : m_name(name) {}
			virtual ~IComponentHandler() = default;

			virtual bool add(Node* node) = 0;

			[[nodiscard]] const std::string& name() const { return m_name; }

		protected:

			std::string m_name;

		};

		template<typename T>
		class ComponentHandler : public IComponentHandler
		{
		public:

			ComponentHandler(const std::string& name) : IComponentHandler(name) {}
			~ComponentHandler() override = default;

			bool add(Node* node) override
			{
				if (!node->has_component<T>())
				{
					node->add_component<T>();

					return true;
				}

				return false;
			}

		private:



		};

	public:

		explicit UIWindowNodeEditor(const std::shared_ptr<core::Engine>& engine);
		~UIWindowNodeEditor() override {}

		void draw(double dt) override;

		//inline void SetEntity(ECS::EntityID entity_) { m_entity = entity_; };
		void set_file_browser(ImGui::FileBrowser* fileDialog) { m_file_dialog = fileDialog; }

		[[nodiscard]] bool scene_changed() const { return m_scene_changed; }

		template<typename T>
		void add_component_type(const std::string& name)
		{
			auto handler = new ComponentHandler<T>(name);

			m_component_handlers.push_back(static_cast<IComponentHandler*>(handler));
		}

	private:

		//ECS::EntityID m_entity = 0;
		ImGui::FileBrowser* m_file_dialog = nullptr;

		bool m_model_selected = false;
		bool m_texture_selected = false;
		bool m_scene_changed = false;

		std::vector<IComponentHandler*> m_component_handlers;

		void draw_transform_ui_2d_node(ImGuiTreeNodeFlags flags, Node* node);
		void draw_transform_ui_3d_node(ImGuiTreeNodeFlags flags, Node* node);

		void draw_mesh_ui(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::MeshComponent& mesh);
		void draw_light_ui(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent& light);
		void draw_shadowcaster_ui(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent& shadow);

		void draw_procedural_plane_ui(ImGuiTreeNodeFlags flags, entt::entity entity, procedural::PlaneComponent& plane);

		void draw_rigidbody_2d_ui(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody);
		void draw_circle_2d_ui(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle);
		void draw_box_2d_ui(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box);

		void draw_script_ui(ImGuiTreeNodeFlags flags, entt::entity entity, scripting::AngelScriptComponent& script);
	};
}
