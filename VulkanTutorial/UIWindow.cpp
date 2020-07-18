#include "UIWindow.h"

namespace Puffin
{
	namespace UI
	{
		UIWindow::UIWindow()
		{
			show = true;
			windowSize = ImVec2(0, 0);
		}

		UIWindow::~UIWindow()
		{

		}

		bool UIWindow::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			return true;
		}

		bool UIWindow::Begin(std::string name)
		{
			return ImGui::Begin(name.c_str(), &show, flags);
		}

		void UIWindow::End()
		{
			ImGui::End();
		}
	}
}