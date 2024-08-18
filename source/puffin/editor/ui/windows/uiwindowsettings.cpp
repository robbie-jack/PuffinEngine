#include "puffin/editor/ui/windows/uiwindowsettings.h"

#include "puffin/core/engine.h"
#include "puffin/core/settingsmanager.h"

namespace puffin
{
	namespace ui
	{
		void UIWindowSettings::draw(double dt)
		{
			if (mFirstTime)
			{
				mWindowName = "Settings";

				mFirstTime = false;
			}

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				begin(mWindowName);

                auto settings_manager = m_engine->GetSubsystem<core::SettingsManager>();

                auto mouse_sensitivity = settings_manager->Get<float>("mouse_sensitivity");
				if (ImGui::DragFloat("Mouse Sensitivity", &mouse_sensitivity, 0.001f, 0.01f, 0.1f))
				{
                    settings_manager->Set("mouse_sensitivity", mouse_sensitivity);
				}

                auto editor_camera_fov = settings_manager->Get<float>("editor_camera_fov");
				if (ImGui::DragFloat("Editor Camera FOV", &editor_camera_fov, 0.5f, 30.0f, 120.0f))
				{
					//mCamera->fov_y = editor_camera_fov;
                    settings_manager->Set("editor_camera_fov", editor_camera_fov);
				}

				end();
			}
		}
	}
}