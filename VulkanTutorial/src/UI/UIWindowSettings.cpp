#include <UI/UIWindowSettings.h>

namespace Puffin
{
	namespace UI
	{
		bool UIWindowSettings::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (firstTime)
			{
				windowName = "Settings";

				// Load Settings from file
				settings = IO::LoadSettings("projectsettings.xml");
				InputManager->GetSensitivity() = settings.mouseSensitivity;
				camera->fov = settings.cameraFov;

				firstTime = false;
			}

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(windowName);

				if (ImGui::SliderFloat("Sensitivity", &settings.mouseSensitivity, 0.01f, 0.1f))
				{
					InputManager->GetSensitivity() = settings.mouseSensitivity;
				}

				if (ImGui::SliderFloat("Field of View", &settings.cameraFov, 30.0f, 120.0f, "%f"))
				{
					camera->fov = settings.cameraFov;
				}

				if (ImGui::Button("Save"))
				{
					IO::SaveSettings("projectsettings.xml", settings);
				}

				End();
			}

			return true;
		}
	}
}