#include "UIWindowSettings.h"
#include <Engine.h>

namespace Puffin
{
	namespace UI
	{
		void UIWindowSettings::Draw(float dt)
		{
			if (firstTime)
			{
				windowName = "Settings";

				firstTime = false;
			}

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(windowName);

				IO::ProjectSettings& settings = m_engine->GetProjectSettings();

				if (ImGui::SliderFloat("Sensitivity", &settings.mouseSensitivity, 0.01f, 0.1f))
				{
					m_inputManager->GetSensitivity() = settings.mouseSensitivity;
				}

				if (ImGui::SliderFloat("Field of View", &settings.cameraFov, 30.0f, 120.0f, "%f"))
				{
					m_camera->fov = settings.cameraFov;
				}

				End();
			}
		}
	}
}