#include "UIWindowPerformance.h"
#include <thread>

namespace Puffin
{
	namespace UI
	{
		bool UIWindowPerformance::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			windowName = "Performance";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				if (!Begin(windowName))
				{
					End();
				}
				else
				{
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("System Info"))
					{
						hardwareStats.cpuName = "Ryzen 3600";
						hardwareStats.logicalCores = std::thread::hardware_concurrency();
						hardwareStats.physicalCores = hardwareStats.logicalCores / 2;
						hardwareStats.gpuName = "GTX 1070";
						hardwareStats.vramTotal = 16384;
						hardwareStats.ramTotal = 16384;

						ImGui::Text(" CPU: %s", hardwareStats.cpuName);
						ImGui::Text(" Physical Cores: %d", hardwareStats.physicalCores);
						ImGui::Text(" Logical Cores: %d", hardwareStats.logicalCores);
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						ImGui::Text(" GPU: %s", hardwareStats.gpuName);
						ImGui::Text(" VRAM: %d MB", hardwareStats.vramTotal);
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						ImGui::Text(" System Memory: %d MB", hardwareStats.ramTotal);
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
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

						ImGui::Text(" Framerate: %d", fps);

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