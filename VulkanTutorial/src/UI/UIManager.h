#pragma once

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <imgui.h>
#include <backends\imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imfilebrowser.h>

#include <Input/InputManager.h>
#include <ECS/ECS.h>
#include <Engine.h>

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
	namespace UI
	{
		class UIManager
		{
		public:

			UIManager();
			~UIManager();

			void Cleanup();

			bool DrawUI(float dt, Puffin::Input::InputManager* InputManager);
			void Update();
			void AddWindow(UIWindow* window);

			inline void SetEngine(Engine* engine_) { engine = engine_; };

			inline UIWindowViewport* GetWindowViewport() { return windowViewport; };
			inline UIWindowSettings* GetWindowSettings() { return windowSettings; };

			inline void SetWorld(ECS::World* world_) 
			{ 
				world = world_;
				windowSceneHierarchy->SetWorld(world_);
				windowEntityProperties->SetWorld(world_);
				windowViewport->SetWorld(world_);
			};

		private:
			bool running;
			bool saveScene, loadScene, importMesh;

			std::string playButtonLabel;

			Engine* engine;
			ECS::World* world;
			ECS::Entity entity;

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