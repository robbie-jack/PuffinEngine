#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "InputManager.h"

namespace Puffin
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