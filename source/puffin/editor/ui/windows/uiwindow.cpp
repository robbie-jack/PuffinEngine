#include "puffin/editor/ui/windows/uiwindow.h"

namespace puffin
{
	namespace ui
	{
		UIWindow::UIWindow(const std::shared_ptr<core::Engine>& engine): m_engine(engine)
		{
			mShow = true;
			mFirstTime = true;
			mFlags = ImGuiWindowFlags_None;
		}

		UUID UIWindow::GetSelectedEntity() const
		{
			return mSelectedEntity;
		}

		void UIWindow::SetSelectedEntity(const UUID selectedEntity)
		{
			mSelectedEntity = selectedEntity;
		}

		bool UIWindow::GetShow() const
		{
			return mShow;
		}

		void UIWindow::Show()
		{
			mShow = !mShow;
		}

		const std::string& UIWindow::GetName()
		{
			return mWindowName;
		}

		bool UIWindow::Begin(const std::string& name)
		{
			return ImGui::Begin(name.c_str(), &mShow, mFlags);
		}

		void UIWindow::End()
		{
			ImGui::End();
		}
	}
}
