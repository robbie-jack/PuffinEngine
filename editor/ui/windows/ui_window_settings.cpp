#include "ui/windows/ui_window_settings.h"

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

				ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue;

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("General"))
				{
					auto framerateLimitEnable = settingsManager->Get<bool>("general", "framerate_limit_enable").value_or(true);
					if (ImGui::Checkbox("Framerate Limit Enabled", &framerateLimitEnable))
					{
						settingsManager->Set("general", "framerate_limit_enable", framerateLimitEnable);
					}

					{
						int framerateLimitMin = 30;
						int framerateLimitMax = 1000;
						
						ImGui::SetNextItemWidth(200.0f);
						
						auto framerateLimit = settingsManager->Get<int>("general", "framerate_limit").value_or(300);
						if (ImGui::SliderInt("##FramerateLimitSlider", &framerateLimit, framerateLimitMin, framerateLimitMax))
						{
							settingsManager->Set("general", "framerate_limit", framerateLimit);
						}

						ImGui::SameLine();

						ImGui::SetNextItemWidth(100.0f);
						if (ImGui::InputInt("Framerate Limit", &framerateLimit, 1, 10, inputTextFlags))
						{
							framerateLimit = std::max(framerateLimit, framerateLimitMin);
							framerateLimit = std::min(framerateLimit, framerateLimitMax);
							
							settingsManager->Set("general", "framerate_limit", framerateLimit);
						}
					}
					
					auto mouseSensitivity = settingsManager->Get<float>("general", "mouse_sensitivity").value_or(0.05f);
					if (ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.001f, 0.01f))
					{
						settingsManager->Set("general", "mouse_sensitivity", mouseSensitivity);
					}
				}
				
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("Physics"))
				{
					{
						int ticksPerSecondMin = 30;
						int ticksPerSecondMax = 300;
						
						ImGui::SetNextItemWidth(200.0f);
						
						auto ticksPerSecond = settingsManager->Get<int>("physics", "ticks_per_second").value_or(60);
						if (ImGui::SliderInt("##TicksPerSecondSlider", &ticksPerSecond, ticksPerSecondMin, ticksPerSecondMax))
						{
							settingsManager->Set("physics", "ticks_per_second", ticksPerSecond);
						}

						ImGui::SameLine();

						ImGui::SetNextItemWidth(100.0f);
						if (ImGui::InputInt("Ticks Per Second", &ticksPerSecond, 1, 10, inputTextFlags))
						{
							ticksPerSecond = std::max(ticksPerSecond, ticksPerSecondMin);
							ticksPerSecond = std::min(ticksPerSecond, ticksPerSecondMax);
							
							settingsManager->Set("physics", "ticks_per_second", ticksPerSecond);
						}
					}
				}

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("Rendering"))
				{
					
				}

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::CollapsingHeader("Editor"))
                {
                	auto editorCameraFov = settingsManager->Get<float>("editor", "camera_fov").value_or(60.0f);
                	if (ImGui::DragFloat("Editor Camera FOV", &editorCameraFov, 0.5f, 30.0f, 120.0f))
                	{
                		settingsManager->Set("editor", "camera_fov", editorCameraFov);
                	}
                }

				End();
			}
		}
	}
}