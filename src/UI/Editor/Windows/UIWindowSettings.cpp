#include "UI/Editor/Windows/UIWindowSettings.h"

#include "Engine/Engine.hpp"

namespace Puffin
{
	namespace UI
	{
		void UIWindowSettings::Draw(double dt)
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

				auto input = m_engine->GetSubsystem<Input::InputSubsystem>();

				if (ImGui::SliderFloat("Sensitivity", &settings.mouseSensitivity, 0.01f, 0.1f))
				{
					input->GetSensitivity() = settings.mouseSensitivity;
				}

				if (ImGui::SliderFloat("Field of View", &settings.cameraFov, 30.0f, 120.0f, "%f"))
				{
					m_camera->fovY = settings.cameraFov;
				}

				End();
			}
		}
	}
}