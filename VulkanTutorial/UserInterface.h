#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "InputManager.h"

namespace Puffin
{
	namespace UI
	{
		class UserInterface
		{
		public:

			UserInterface();
			~UserInterface();

			bool DrawUI(float dt, Puffin::Input::InputManager* InputManager);

		private:
			bool running;

			float fps;
			float fps_timer;
		};
	}
}