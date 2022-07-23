#include "UIWindow.h"

namespace Puffin
{
	namespace UI
	{
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