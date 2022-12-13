#pragma once

#include <imgui.h>
#include <backends\imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imfilebrowser.h>

#include <Input/InputSubsystem.h>

#include "UI/Editor/Windows/UIWindow.h"
#include "UI/Editor/Windows/UIWindowSceneHierarchy.h"
#include "UI/Editor/Windows/UIWindowViewport.h"
#include "UI/Editor/Windows/UIWindowSettings.h"
#include "UI/Editor/Windows/UIWindowEntityProperties.h"
#include "UI/Editor/Windows/UIWindowPerformance.h"
#include "UI/Editor/Windows/UIContentBrowser.h"

#include <vector>
#include <memory>

namespace Puffin
{
	namespace ECS
	{
		class World;
		typedef UUID EntityID;
	}

	namespace Core
	{
		class Engine;
	}

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

			UIManager(std::shared_ptr<Core::Engine> engine);
			~UIManager() {}

			void Cleanup();

			void DrawUI(double dt);
			void Update();
			void AddWindow(std::shared_ptr<UIWindow> window);

			inline std::shared_ptr<UIWindowViewport> GetWindowViewport() { return windowViewport; }
			inline std::shared_ptr<UIWindowSettings> GetWindowSettings() { return windowSettings; }

		private:
			bool saveScene, loadScene;
			ImportAssetUI importAssetUI;

			std::shared_ptr<Core::Engine> m_engine = nullptr;
			ECS::EntityID m_entity;

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