#pragma once

#include <string>

#include "imgui/imgui.h"
#include "InputManager.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindow
		{
		public:

			UIWindow();
			~UIWindow();

			virtual bool Draw(float dt, Puffin::Input::InputManager* InputManager);

			void Show();

		protected:

			virtual bool Begin(std::string name);
			void End();

			bool show;

			ImGuiWindowFlags flags;
			ImVec2 windowSize;
		};
	}
}