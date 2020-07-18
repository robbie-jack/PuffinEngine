#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "InputManager.h"
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

			bool DrawUI(float dt, Puffin::Input::InputManager* InputManager);
			void AddWindow(UIWindow* window);

		private:
			bool running;

			std::vector<UIWindow*> windows;

			bool ShowDockspace(bool* p_open);
			void SetStyle();
		};
	}
}