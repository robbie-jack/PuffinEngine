#include "puffin/editor/ui/windows/uiwindowperformance.h"

#include <thread>
#include <math.h>
#include <stdio.h> 

#include "imgui.h"
#include "puffin/utility/performancebenchmarksubsystem.h"

namespace puffin
{
	namespace ui
	{
		constexpr int s_max_benchmark_values = 100;

		void UIWindowPerformance::draw(double dt)
		{
			mWindowName = "Performance";

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				begin(mWindowName);

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("System Info"))
				{
					hardwareStats.cpuName = "Ryzen 3600";
					hardwareStats.logicalCores = std::thread::hardware_concurrency();
					hardwareStats.physicalCores = hardwareStats.logicalCores / 2;
					hardwareStats.gpuName = "GTX 3070";
					hardwareStats.vramTotal = 8192;
					hardwareStats.ramTotal = 16384;

					ImGui::Text(" CPU: %s", hardwareStats.cpuName.c_str());
					ImGui::Text(" Physical Cores: %d", hardwareStats.physicalCores);
					ImGui::Text(" Logical Cores: %d", hardwareStats.logicalCores);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::Text(" GPU: %s", hardwareStats.gpuName.c_str());
					ImGui::Text(" VRAM: %.1f GB", hardwareStats.vramTotal / 1024.0f);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::Text(" System Memory: %.1f GB", hardwareStats.ramTotal / 1024.0f);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
				}

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("Performance Metrics"))
				{
					// Display FPS
					mFpsTimer += dt;

					constexpr int numValues = 120;
					static int valueOffset = 0;

					static float framerateValues[numValues] = {};
					static float framerateMin = 10000.0f;
					static float framerateMax = 0.0f;

					static float frametimeValues[numValues] = {};
					static float frametimeMin = 1000.0f;
					static float frametimeMax = 0.0f;

					if (constexpr float refreshTime = 1 / 60.0f; mFpsTimer >= refreshTime)
					{
						mFps = 1 / dt;
						mFrametime = dt * 1000;
						mFpsTimer = 0.0;

						if (mFps > framerateMax)
							framerateMax = mFps;

						if (mFps < framerateMin)
							framerateMin = mFps;

						if (mFrametime > frametimeMax)
							frametimeMax = mFrametime;

						if (mFrametime < frametimeMin)
							frametimeMin = mFrametime;

						framerateValues[valueOffset] = mFps;
						frametimeValues[valueOffset] = mFrametime;
						valueOffset = (valueOffset + 1) % numValues;
					}

					// Display Framerate
					float framerateAverage = 0.0f;
					for (const float framerateValue : framerateValues)
					{
						framerateAverage += framerateValue;
					}
					framerateAverage /= static_cast<float>(numValues);

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::PlotLines("Framerate", framerateValues, numValues, valueOffset, nullptr, 0.0f, 144.0f,
					                 ImVec2(0.0f, 80.0f));

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Current: %.1f", mFps);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Average: %.1f", framerateAverage);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Minimum: %.1f", framerateMin);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Maximum: %.1f", framerateMax);

					ImGui::NewLine();

					// Display Frametime
					float frametimeAverage = 0.0f;
					for (const float frametimeValue : frametimeValues)
					{
						frametimeAverage += frametimeValue;
					}
					frametimeAverage /= static_cast<float>(numValues);

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::PlotLines("Frametime", frametimeValues, numValues, valueOffset, nullptr, 0.0f, 100.0f,
					                 ImVec2(0.0f, 80.0f));

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Current: %.1f", mFrametime);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Average: %.1f", frametimeAverage);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Minimum: %.1f", frametimeMin);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Maximum: %.1f", frametimeMax);

					ImGui::NewLine();

					// Display Stage/System Frametime breakdown
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Frametime Breakdown");
					ImGui::NewLine();

					auto benchmark_subsystem = m_engine->get_subsystem<utility::PerformanceBenchmarkSubsystem>();

					draw_benchmark("Input", benchmark_subsystem->get_benchmark_time("Input"));
					draw_benchmark("Sample Time", benchmark_subsystem->get_benchmark_time("Sample Time"));

					draw_benchmark("Engine Update", benchmark_subsystem->get_benchmark_time("Engine Update"));

					ImGui::Indent();
					for (const auto& name : benchmark_subsystem->get_category("Engine Update"))
					{
						draw_benchmark(name, benchmark_subsystem->get_benchmark_time_category(name, "Engine Update"));
					}
					ImGui::Unindent();

					draw_benchmark("Fixed Update", benchmark_subsystem->get_benchmark_time("Fixed Update"));

					ImGui::Indent();
					for (const auto& name : benchmark_subsystem->get_category("Fixed Update"))
					{
						draw_benchmark(name, benchmark_subsystem->get_benchmark_time_category(name, "Fixed Update"));
					}
					ImGui::Unindent();

					draw_benchmark("Update", benchmark_subsystem->get_benchmark_time("Update"));

					ImGui::Indent();
					for (const auto& name : benchmark_subsystem->get_category("Update"))
					{
						draw_benchmark(name, benchmark_subsystem->get_benchmark_time_category(name, "Update"));
					}
					ImGui::Unindent();

					draw_benchmark("Render", benchmark_subsystem->get_benchmark_time("Render"));
				}

				end();
			}
		}

		void UIWindowPerformance::draw_benchmark(const std::string& name, double benchmark_time)
		{
			auto benchmark_subsystem = m_engine->get_subsystem<utility::PerformanceBenchmarkSubsystem>();

			if (m_benchmark_idx.find(name) == m_benchmark_idx.end())
			{
				m_benchmark_idx.emplace(name, 0);

				m_benchmark_values.emplace(name, std::vector<double>());
				m_benchmark_values.at(name).reserve(s_max_benchmark_values);
			}

			auto& benchmark_vector = m_benchmark_values.at(name);
			if(benchmark_vector.size() < s_max_benchmark_values)
			{
				benchmark_vector.push_back(0.0);
			}

			benchmark_vector[m_benchmark_idx.at(name) % s_max_benchmark_values] = benchmark_time;

			double benchmark_average = 0.0;
			for (auto value : benchmark_vector)
			{
				benchmark_average += value;
			}
			benchmark_average /= benchmark_vector.size();

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::SameLine();
			ImGui::Text("%s: %.3f ms", name.c_str(), benchmark_average);

			//ImGui::NewLine();

			m_benchmark_idx[name] = m_benchmark_idx[name] + 1 % s_max_benchmark_values;
		}
	}
}
