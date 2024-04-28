#include "puffin/ui/editor/windows/ui_window_settings.h"

#include "puffin/core/engine.h"

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

				io::ProjectSettings& settings = mEngine->settings();

				const auto input = mEngine->getSystem<input::InputSubsystem>();

				if (ImGui::SliderFloat("Sensitivity", &settings.mouseSensitivity, 0.01f, 0.1f))
				{
					input->sensitivity() = settings.mouseSensitivity;
				}

				if (ImGui::SliderFloat("Field of View", &settings.cameraFov, 30.0f, 120.0f, "%f"))
				{
					mCamera->fovY = settings.cameraFov;
				}

				end();
			}
		}
	}
}