#include "puffin/editor/ui/windows/uiwindowsettings.h"

#include "puffin/core/engine.h"
#include "puffin/core/settingsmanager.h"

namespace puffin
{
	namespace ui
	{
		UIWindowSettings::UIWindowSettings(std::shared_ptr<core::Engine> engine): UIWindow(engine)
		{
			
		}

		void UIWindowSettings::Draw(double deltaTime)
		{
			if (mFirstTime)
			{
				mWindowName = "Settings";

				mFirstTime = false;
			}

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(mWindowName);

				const auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();
				
                auto mouseSensitivity = settingsManager->Get<float>("general", "mouse_sensitivity").value_or(0.05f);
				if (ImGui::DragFloat("Mouse Sensitivity", &mouseSensitivity, 0.001f, 0.01f, 0.1f))
				{
                    settingsManager->Set("general", "mouse_sensitivity", mouseSensitivity);
				}

                auto editorCameraFov = settingsManager->Get<float>("editor", "camera_fov").value_or(60.0f);
				if (ImGui::DragFloat("Editor Camera FOV", &editorCameraFov, 0.5f, 30.0f, 120.0f))
				{
                    settingsManager->Set("editor", "camera_fov", editorCameraFov);
				}

				End();
			}
		}
	}
}