#pragma once

#include <memory>

#include "puffin/editor/ui/windows/uiwindow.h"
#include "puffin/ecs/enttsubsystem.h"
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

			virtual [[nodiscard]] const std::string& GetName() const;

		protected:

			

		};

		template<typename T>
		class ComponentHandler : public IComponentHandler
		{
		public:

			ComponentHandler(const std::string& name) : mName(name) {}
			~ComponentHandler() override = default;

			bool Add(Node* node) override
			{
				if (!node->has_component<T>())
				{
					node->add_component<T>();

					return true;
				}

				return false;
			}

			[[nodiscard]] const std::string& GetName() const override { return mName; };

		private:

			std::string mName;

		};

	public:

		explicit UIWindowNodeEditor(const std::shared_ptr<core::Engine>& engine);
		~UIWindowNodeEditor() override = default;

		void draw(double dt) override;

		//inline void SetEntity(ECS::EntityID entity_) { m_entity = entity_; };
		void SetFileBrowser(ImGui::FileBrowser* fileDialog) { mFileDialog = fileDialog; }

		[[nodiscard]] bool GetSceneChanged() const { return mSceneChanged; }

		template<typename T>
		void AddComponentType(const std::string& name)
		{
			auto handler = new ComponentHandler<T>(name);

			mComponentHandlers.push_back(static_cast<IComponentHandler*>(handler));
		}

	private:

		//ECS::EntityID m_entity = 0;
		ImGui::FileBrowser* mFileDialog = nullptr;

		bool mModelSelected = false;
		bool mTextureSelected = false;
		bool mSceneChanged = false;

		std::vector<IComponentHandler*> mComponentHandlers;

		void DrawTransformUI2DNode(ImGuiTreeNodeFlags flags, Node* node);
		void DrawTransformUI3DNode(ImGuiTreeNodeFlags flags, Node* node);

		void DrawMeshUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::StaticMeshComponent3D& mesh);
		void DrawLightUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent3D& light);
		void DrawShadowcasterUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent3D& shadow);

		void DrawProceduralPlaneUI(ImGuiTreeNodeFlags flags, entt::entity entity, procedural::ProceduralPlaneComponent3D& plane);

		void DrawRigidbody2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody);
		void DrawCircle2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle);
		void DrawBox2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box);

		void DrawScriptUI(ImGuiTreeNodeFlags flags, entt::entity entity, scripting::AngelScriptComponent& script);
	};
}
