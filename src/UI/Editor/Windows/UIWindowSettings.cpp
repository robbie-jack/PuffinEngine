#include "UI/Editor/Windows/UIWindowSettings.h"

#include "Core/Engine.h"

namespace puffin
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

				io::ProjectSettings& settings = m_engine->settings();

				auto input = m_engine->getSubsystem<input::InputSubsystem>();

				if (ImGui::SliderFloat("Sensitivity", &settings.mouseSensitivity, 0.01f, 0.1f))
				{
					input->sensitivity() = settings.mouseSensitivity;
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