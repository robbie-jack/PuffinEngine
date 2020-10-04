#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/implot.h"

#include "InputManager.h"
#include "Engine.h"
#include "UIWindow.h"

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

		private:
			bool running;
			std::string playButtonLabel;

			Engine* engine;
			std::vector<UIWindow*> windows;

			bool ShowDockspace(bool* p_open);
			void SetStyle();
		};
	}
}