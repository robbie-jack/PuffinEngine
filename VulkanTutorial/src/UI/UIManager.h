#pragma once

#include <imgui.h>
#include <backends\imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imfilebrowser.h>

#include <Input/InputManager.h>

#include "Windows/UIWindow.h"
#include "Windows/UIWindowSceneHierarchy.h"
#include "UI/Windows/UIWindowViewport.h"
#include "UI/Windows/UIWindowSettings.h"
#include "UI/Windows/UIWindowEntityProperties.h"
#include "UI/Windows/UIWindowPerformance.h"
#include "Windows/UIContentBrowser.h"

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

			UIManager(Engine* InEngine, std::shared_ptr<ECS::World> InWorld, std::shared_ptr<Input::InputManager> InInput);
			~UIManager();

			void Cleanup();

			void DrawUI(float dt, std::shared_ptr<Input::InputManager> InputManager);
			void Update();
			void AddWindow(std::shared_ptr<UIWindow> window);

			inline std::shared_ptr<UIWindowViewport> GetWindowViewport() { return windowViewport; }
			inline std::shared_ptr<UIWindowSettings> GetWindowSettings() { return windowSettings; }

		private:
			bool saveScene, loadScene;
			ImportAssetUI importAssetUI;

			Engine* m_engine;
			std::shared_ptr<ECS::World> m_world;
			ECS::Entity m_entity;

			std::vector<std::shared_ptr<UIWindow>> m_windows;

			std::shared_ptr <UIWindowSceneHierarchy> windowSceneHierarchy;
			std::shared_ptr<UIWindowViewport> windowViewport;
			std::shared_ptr<UIWindowSettings> windowSettings;
			std::shared_ptr<UIWindowEntityProperties> windowEntityProperties;
			std::shared_ptr<UIWindowPerformance> windowPerformance;
			std::shared_ptr<UIContentBrowser> contentBrowser;

			ImGui::FileBrowser fileDialog;

			void ShowDockspace(bool* p_open);
			void ShowMenuBar();
			void SetStyle();
		};
	}
}