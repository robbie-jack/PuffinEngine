#include "puffin/ui/editor/windows/ui_window.h"

namespace puffin
{
	namespace ui
	{
		void UIWindow::setShow()
		{
			mShow = !mShow;
		}

		bool UIWindow::begin(const std::string name)
		{
			return ImGui::Begin(name.c_str(), &mShow, mFlags);
		}

		void UIWindow::end()
		{
			ImGui::End();
		}
	}
}
