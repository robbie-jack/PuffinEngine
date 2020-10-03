#include "UIWindowPerformance.h"
#include <thread>

namespace Puffin
{
	namespace UI
	{
		bool UIWindowPerformance::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				windowName = "Performance";
				if (!Begin(windowName))
				{
					End();
				}
				else
				{
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("System Info"))
					{
						std::string cpuName = "Ryzen 3600";
						int logicalCores = std::thread::hardware_concurrency();
						int physicalCores = logicalCores / 2;

						ImGui::Text("CPU: %s", cpuName);
						ImGui::Text("Physical Cores: %d", physicalCores);
						ImGui::Text("Logical Cores: %d", logicalCores);
						ImGui::Text("");
					}

					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Performance Metrics"))
					{
						// Display FPS
						fps_timer += dt;

						if (fps_timer >= 0.25f)
						{
							fps = 1 / dt;
							fps_timer = 0.0f;
						}

						ImGui::Text("Framerate: %d", fps);

						plotBuffer.AddPoint(dt, (float)fps);

						/*if (ImPlot::BeginPlot("Framerate", "Time", "FPS"))
						{
							ImPlot::PlotLine("FPS", &plotBuffer.Data[0], plotBuffer.Data.size());
							ImPlot::EndPlot();
						}*/
					}
				}
			}

			return true;
		}
	}
}