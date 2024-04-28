#pragma once

#include "UIWindow.h"

#include "imfilebrowser.h"
#include "ECS/EnTTSubsystem.h"

#include <memory>

namespace puffin
{
	class Node;

	namespace scene
	{
		class SceneGraph;
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
	class UIWindowEntityProperties : public UIWindow
	{
	public:

		UIWindowEntityProperties(const std::shared_ptr<core::Engine>& engine) : UIWindow(engine) {}
		~UIWindowEntityProperties() override {}

		void draw(double dt) override;

		//inline void SetEntity(ECS::EntityID entity_) { m_entity = entity_; };
		void setFileBrowser(ImGui::FileBrowser* fileDialog) { mFileDialog = fileDialog; }

		[[nodiscard]] bool sceneChanged() const { return mSceneChanged; }

	private:

		//ECS::EntityID m_entity = 0;
		ImGui::FileBrowser* mFileDialog = nullptr;

		bool mModelSelected = false;
		bool mTextureSelected = false;
		bool mSceneChanged = false;

		std::shared_ptr<ecs::EnTTSubsystem> mEnTTSubsystem = nullptr;
		std::shared_ptr<scene::SceneGraph> m_scene_graph = nullptr;

		void draw_transform_ui_2d_node(ImGuiTreeNodeFlags flags, Node* node);
		void draw_transform_ui_3d_node(ImGuiTreeNodeFlags flags, Node* node);

		void drawMeshUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::MeshComponent& mesh);
		void drawLightUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent& light);
		void drawShadowcasterUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent& shadowcaster);

		void drawProceduralPlaneUI(ImGuiTreeNodeFlags flags, entt::entity entity, procedural::PlaneComponent& plane);

		void drawRigidbody2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody);
		void drawCircle2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle);
		void drawBox2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box);

		void drawScriptUI(ImGuiTreeNodeFlags flags, entt::entity entity, scripting::AngelScriptComponent& script);
	};
}
