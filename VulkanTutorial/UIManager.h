#pragma once

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
//#include "imgui/implot.h"
#include "imgui/imfilebrowser.h"

#include "InputManager.h"
#include "ECS.h"
#include "Engine.h"
#include "UIWindow.h"

#include "UIWindowSceneHierarchy.h"
#include "UIWindowViewport.h"
#include "UIWindowSettings.h"
#include "UIWindowEntityProperties.h"
#include "UIWindowPerformance.h"

#include <vector>
#include <memory>

namespace Puffin
{
	namespace UI
	{
		class UIManager
		{
		public:

			UIManager();
			~UIManager();

			void Cleanup();

			bool DrawUI(float dt, Puffin::Input::InputManager* InputManager);
			void AddWindow(UIWindow* window);

			inline void SetEngine(Engine* engine_) { engine = engine_; };

			inline UIWindowViewport* GetWindowViewport() { return windowViewport; };
			inline UIWindowSettings* GetWindowSettings() { return windowSettings; };

			inline void SetWorld(ECS::World* world_) 
			{ 
				world = world_;
				windowSceneHierarchy->SetWorld(world_);
				windowEntityProperties->SetWorld(world_);
			};

		private:
			bool running;
			std::string playButtonLabel;

			Engine* engine;
			ECS::World* world;

			std::vector<UIWindow*> windows;

			UIWindowSceneHierarchy* windowSceneHierarchy;
			UIWindowViewport* windowViewport;
			UIWindowSettings* windowSettings;
			UIWindowEntityProperties* windowEntityProperties;
			UIWindowPerformance* windowPerformance;

			ImGui::FileBrowser fileDialog;

			bool ShowDockspace(bool* p_open);
			void SetStyle();
		};
	}
}

#endif // UI_MANAGER_H