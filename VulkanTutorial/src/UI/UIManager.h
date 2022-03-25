#pragma once

#include <imgui.h>
#include <backends\imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imfilebrowser.h>

#include <Input/InputManager.h>

#include <UI/UIWindow.h>
#include <UI/UIWindowSceneHierarchy.h>
#include <UI/UIWindowViewport.h>
#include <UI/UIWindowSettings.h>
#include <UI/UIWindowEntityProperties.h>
#include <UI/UIWindowPerformance.h>

#include <vector>
#include <memory>

namespace Puffin
{
	namespace ECS
	{
		class World;
		typedef uint32_t Entity;
	}

	class Engine;

	namespace UI
	{
		enum class ImportAssetUI
		{
			None,
			Mesh,
			Texture
		};

		class UIManager
		{
		public:

			UIManager(Engine* InEngine, std::shared_ptr<ECS::World> InWorld);
			~UIManager();

			void Cleanup();

			void DrawUI(float dt, Puffin::Input::InputManager* InputManager);
			void Update();
			void AddWindow(UIWindow* window);

			inline UIWindowViewport* GetWindowViewport() { return windowViewport; };
			inline UIWindowSettings* GetWindowSettings() { return windowSettings; };

		private:
			bool saveScene, loadScene;
			ImportAssetUI importAssetUI;

			Engine* engine;
			std::shared_ptr<ECS::World> world;
			ECS::Entity entity;

			std::vector<UIWindow*> windows;

			UIWindowSceneHierarchy* windowSceneHierarchy;
			UIWindowViewport* windowViewport;
			UIWindowSettings* windowSettings;
			UIWindowEntityProperties* windowEntityProperties;
			UIWindowPerformance* windowPerformance;

			ImGui::FileBrowser fileDialog;

			void ShowDockspace(bool* p_open);
			void ShowMenuBar();
			void SetStyle();
		};
	}
}