#include "UI/Editor/Windows/UIWindowPerformance.h"

#include <thread>

#include <math.h>
#include <stdio.h> 

namespace Puffin
{
	namespace UI
	{
		void UIWindowPerformance::Draw(double dt)
		{
			windowName = "Performance";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(windowName);

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("System Info"))
				{
					hardwareStats.cpuName = "Ryzen 3600";
					hardwareStats.logicalCores = std::thread::hardware_concurrency();
					hardwareStats.physicalCores = hardwareStats.logicalCores / 2;
					hardwareStats.gpuName = "GTX 3070";
					hardwareStats.vramTotal = 8192;
					hardwareStats.ramTotal = 16384;

					ImGui::Text(" CPU: %s", hardwareStats.cpuName);
					ImGui::Text(" Physical Cores: %d", hardwareStats.physicalCores);
					ImGui::Text(" Logical Cores: %d", hardwareStats.logicalCores);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::Text(" GPU: %s", hardwareStats.gpuName);
					ImGui::Text(" VRAM: %.1f GB", hardwareStats.vramTotal / 1024.0f);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::Text(" System Memory: %.1f GB", hardwareStats.ramTotal / 1024.0f);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
				}

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("Performance Metrics"))
				{
					// Display FPS
					fps_timer += dt;

					const int num_values = 120;
					static int value_offset = 0;

					static float framerate_values[num_values] = {};
					static float framerate_min = 10000.0f;
					static float framerate_max = 0.0f;

					static float frametime_values[num_values] = {};
					static float frametime_min = 1000.0f;
					static float frametime_max = 0.0f;

					const float refresh_time = 1 / 60.0f;
					if (fps_timer >= refresh_time)
					{
						fps = 1 / dt;
						frametime = dt * 1000;
						fps_timer = 0.0f;

						if (fps > framerate_max)
							framerate_max = fps;

						if (fps < framerate_min)
							framerate_min = fps;

						if (frametime > frametime_max)
							frametime_max = frametime;

						if (frametime < frametime_min)
							frametime_min = frametime;

						framerate_values[value_offset] = fps;
						frametime_values[value_offset] = frametime;
						value_offset = (value_offset + 1) % num_values;
					}

					// Display Framerate
					float framerate_average = 0.0f;
					for (int n = 0; n < num_values; n++)
					{
						framerate_average += framerate_values[n];
					}
					framerate_average /= (float)num_values;

					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine();
					ImGui::PlotLines("Framerate", framerate_values, num_values, value_offset, (const char*)0, 0.0f, 144.0f, ImVec2(0.0f, 80.0f));

					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Current: %.1f", fps);
					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Average: %.1f", framerate_average);
					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Minimum: %.1f", framerate_min);
					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Maximum: %.1f", framerate_max);

					ImGui::NewLine();

					// Display Frametime
					float frametime_average = 0.0f;
					for (int n = 0; n < num_values; n++)
					{
						frametime_average += frametime_values[n];
					}
					frametime_average /= (float)num_values;

					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine();
					ImGui::PlotLines("Frametime", frametime_values, num_values, value_offset, (const char*)0, 0.0f, 100.0f, ImVec2(0.0f, 80.0f));

					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Current: %.1f", frametime);
					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Average: %.1f", frametime_average);
					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Minimum: %.1f", frametime_min);
					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Maximum: %.1f", frametime_max);

					ImGui::NewLine();

					// Display Stage/System Frametime breakdown
					ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("Frametime Breakdown");
					ImGui::NewLine();

					for (const auto& [stage, name] : Core::G_EXECUTION_STAGE_ORDER)
					{
						auto stageFrametime = m_engine->GetStageExecutionTimeLastFrame(stage) * 1000.0;
						ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("%s: %.1f ms", name.c_str(), stageFrametime);

						ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Indent();
						for (const auto& [name, time] : m_engine->GetCallbackExecutionTimeForUpdateStageLastFrame(stage))
						{
							double callbackFrametime = time * 1000.0;
							ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Text("%s: %.1f ms", name.c_str(), callbackFrametime);
						}
						ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::SameLine(); ImGui::Unindent();
					}

					ImGui::NewLine();
				}

				End();
			}
		}
	}
}