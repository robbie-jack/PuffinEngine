#pragma once

#include "UIWindow.h"

#include "imfilebrowser.h"
#include "ECS/EnTTSubsystem.h"

#include <memory>

namespace puffin
{
	namespace rendering
	{
		struct LightComponent;
		struct MeshComponent;
	}

	struct TransformComponent;
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

		void drawTransformUI(ImGuiTreeNodeFlags flags, entt::entity entity, TransformComponent& transform);

		void drawMeshUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::MeshComponent& mesh);

		void drawLightUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent& light);
		void drawShadowcasterUI(ImGuiTreeNodeFlags flags);

		void drawProceduralPlaneUI(ImGuiTreeNodeFlags flags);

		void drawRigidbody2DUI(ImGuiTreeNodeFlags flags);
		void drawCircle2DUI(ImGuiTreeNodeFlags flags);
		void drawBox2DUI(ImGuiTreeNodeFlags flags);

		void drawScriptUI(ImGuiTreeNodeFlags flags);
	};
}
