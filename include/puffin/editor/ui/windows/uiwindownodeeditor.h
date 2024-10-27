#pragma once

#include <memory>
#include <string>

#include "puffin/editor/ui/windows/uiwindow.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/nodes/node.h"

namespace ImGui
{
	class FileBrowser;
}

namespace puffin
{
	class TransformNode3D;
	class TransformNode2D;

	namespace scene
	{
		class SceneGraphSubsystem;
	}

	namespace rendering
	{
		class DirectionalLightNode3D;
		class SpotLightNode3D;
		class PointLightNode3D;
		class StaticMeshNode3D;
		struct DirectionalLightComponent3D;
		struct SpotLightComponent3D;
		struct PointLightComponent3D;
		struct LightComponent3D;
		struct StaticMeshComponent3D;
		struct ShadowCasterComponent3D;
	}

	namespace procedural
	{
		struct ProceduralPlaneComponent3D;
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

			virtual ~IComponentHandler() = default;

			virtual bool Add(Node* node) = 0;

			[[nodiscard]] virtual const std::string& GetName() const = 0;

		};

		template<typename T>
		class ComponentHandler : public IComponentHandler
		{
		public:

			explicit ComponentHandler(std::string name) : mName(std::move(name)) {}
			~ComponentHandler() override = default;

			bool Add(Node* node) override
			{
				if (!node->HasComponent<T>())
				{
					node->AddComponent<T>();

					return true;
				}

				return false;
			}

			[[nodiscard]] const std::string& GetName() const override { return mName; }

		private:

			std::string mName;

		};

	public:

		explicit UIWindowNodeEditor(const std::shared_ptr<core::Engine>& engine);
		~UIWindowNodeEditor() override = default;

		void Draw(double deltaTime) override;

		//inline void SetEntity(ECS::EntityID entity_) { m_entity = entity_; };
		void SetFileBrowser(ImGui::FileBrowser* fileDialog);

		[[nodiscard]] bool GetSceneChanged() const;

		template<typename T>
		void AddComponentType(const std::string& name)
		{
			auto handler = new ComponentHandler<T>(name);

			mComponentHandlers.push_back(static_cast<IComponentHandler*>(handler));
		}

	private:

		/*void DrawTransform2DUINode(ImGuiTreeNodeFlags flags, TransformNode2D* node);
		void DrawTransform3DUINode(ImGuiTreeNodeFlags flags, TransformNode3D* node);

		void DrawStaticMeshNode3DUI(ImGuiTreeNodeFlags flags, rendering::StaticMeshNode3D* node);
		void DrawPointLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::PointLightNode3D* node);
		void DrawSpotLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::SpotLightNode3D* node);
		void DrawDirLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::DirectionalLightNode3D* node);*/


		/*void DrawStaticMesh3DUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::StaticMeshComponent3D& mesh);
		void DrawPointLight3DUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::PointLightComponent3D& light);
		void DrawSpotLight3DUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::SpotLightComponent3D& light);
		void DrawDirectionalLight3DUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::DirectionalLightComponent3D& light);
		void DrawShadowCaster3DUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent3D& shadow);

		void DrawProceduralPlaneUI(ImGuiTreeNodeFlags flags, entt::entity entity, procedural::ProceduralPlaneComponent3D& plane);

		void DrawRigidbody2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody);
		void DrawCircle2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle);
		void DrawBox2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box);*/

		//ECS::EntityID m_entity = 0;
		ImGui::FileBrowser* mFileDialog = nullptr;

		bool mModelSelected = false;
		bool mTextureSelected = false;
		bool mSceneChanged = false;

		std::vector<IComponentHandler*> mComponentHandlers;

		
	};
}
