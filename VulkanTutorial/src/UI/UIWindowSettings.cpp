#include <UI/UIWindowSettings.h>
#include <Engine.h>

namespace Puffin
{
	namespace UI
	{

		UIWindowSettings::UIWindowSettings(Engine* InEngine, ECS::World* InWorld) : UIWindow(InEngine, InWorld)
		{

		}

		bool UIWindowSettings::Draw(float dt, Puffin::Input::InputManager* InputManager)
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

				ProjectSettings& settings = engine->GetProjectSettings();

				if (ImGui::SliderFloat("Sensitivity", &settings.mouseSensitivity, 0.01f, 0.1f))
				{
					InputManager->GetSensitivity() = settings.mouseSensitivity;
				}

				if (ImGui::SliderFloat("Field of View", &settings.cameraFov, 30.0f, 120.0f, "%f"))
				{
					camera->fov = settings.cameraFov;
				}

				End();
			}

			return true;
		}
	}
}