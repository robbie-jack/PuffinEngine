#include "UIWindow.h"

namespace Puffin
{
	namespace UI
	{
		UIWindow::UIWindow(Engine* InEngine, ECS::World* InWorld)
		{
			engine = InEngine;
			world = InWorld;
			show = true;
			firstTime = true;
		}

		UIWindow::~UIWindow()
		{

		}

		bool UIWindow::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			return true;
		}

		void UIWindow::Show()
		{
			show = !show;
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