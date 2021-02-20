#pragma once

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

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
		class UIManager
		{
		public:

			UIManager(Engine* InEngine, ECS::World* InWorld);
			~UIManager();

			void Cleanup();

			void DrawUI(float dt, Puffin::Input::InputManager* InputManager);
			void Update();
			void AddWindow(UIWindow* window);

			inline UIWindowViewport* GetWindowViewport() { return windowViewport; };
			inline UIWindowSettings* GetWindowSettings() { return windowSettings; };

		private:
			bool saveScene, loadScene, importMesh;

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

			void ShowDockspace(bool* p_open);
			void ShowMenuBar();
			void SetStyle();
		};
	}
}

#endif // UI_MANAGER_H