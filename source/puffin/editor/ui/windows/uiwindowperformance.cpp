#include "puffin/editor/ui/windows/uiwindowperformance.h"

#include <imgui_internal.h>
#include <thread>
#include <math.h>
#include <stdio.h> 

#include "imgui.h"
#include "puffin/utility/benchmark.h"

namespace puffin
{
	namespace ui
	{
		constexpr int s_max_benchmark_values = 100;

		UIWindowPerformance::UIWindowPerformance(std::shared_ptr<core::Engine> engine): UIWindow(engine)
		{
		}

		void UIWindowPerformance::Draw(double deltaTime)
		{
			mWindowName = "Performance";

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(mWindowName);

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("System Info"))
				{
					hardwareStats.cpuName = "Ryzen 5600";
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
					mFpsTimer += deltaTime;

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
						mFps = 1 / deltaTime;
						mFrameTime = deltaTime * 1000;
						mFpsTimer = 0.0;

						if (mFps > framerateMax)
							framerateMax = mFps;

						if (mFps < framerateMin)
							framerateMin = mFps;

						if (mFrameTime > frametimeMax)
							frametimeMax = mFrameTime;

						if (mFrameTime < frametimeMin)
							frametimeMin = mFrameTime;

						framerateValues[valueOffset] = mFps;
						frametimeValues[valueOffset] = mFrameTime;
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
					ImGui::PlotLines("Framerate", framerateValues, numValues, valueOffset, nullptr, 0.0f, 165.0f,
					                 ImVec2(0.0f, 160.0f));

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Current: %.3f", mFps);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Average: %.3f", framerateAverage);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Minimum: %.3f", framerateMin);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Maximum: %.3f", framerateMax);

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
					                 ImVec2(0.0f, 160.0f));

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Current: %.3f", mFrameTime);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Average: %.3f", frametimeAverage);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Minimum: %.3f", frametimeMin);
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Maximum: %.3f", frametimeMax);

					ImGui::NewLine();

					// Display Stage/System Frametime breakdown
					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::SameLine();
					ImGui::Text("Frametime Breakdown");
					ImGui::NewLine();

					DrawBenchmark("Input");
					DrawBenchmark("WaitForLastPresentationAndSample");
					DrawBenchmark("Idle");
					DrawBenchmark("EngineUpdate");
					DrawBenchmark("FixedUpdate");
					DrawBenchmark("Update");
					DrawBenchmark("Render");
				}

				End();
			}
		}

		void UIWindowPerformance::DrawBenchmark(const std::string& name)
		{
			auto* benchmarkManager = utility::BenchmarkManager::Get();
			auto* benchmark = benchmarkManager->Get(name);

			if (benchmark)
			{
				double benchmarkTimeElapsed = benchmark->GetData().timeElapsed;
			
				if (mBenchmarkIdx.find(name) == mBenchmarkIdx.end())
				{
					mBenchmarkIdx.emplace(name, 0);

					mBenchmarkValues.emplace(name, std::vector<double>());
					mBenchmarkValues.at(name).reserve(s_max_benchmark_values);
				}

				const int benchmarkIdx = mBenchmarkIdx.at(name) % s_max_benchmark_values;

				auto& benchmarkVector = mBenchmarkValues.at(name);
				if(benchmarkVector.size() < s_max_benchmark_values)
				{
					benchmarkVector.push_back(benchmarkTimeElapsed);
				}
				else
				{
					benchmarkVector[benchmarkIdx] = benchmarkTimeElapsed;
				}

				double benchmarkAverage = 0.0;
				for (int i = 0; i < benchmarkVector.size(); ++i)
				{
					benchmarkAverage += benchmarkVector[i];
				}
				benchmarkAverage /= benchmarkVector.size();

				if (ImGui::CollapsingHeader(name.c_str()))
				{
					for (const auto& [benchmarkName, benchmarkChild] : benchmark->GetBenchmarks())
					{
						DrawBenchmark(benchmarkName);
					}
				}
			
				//ImGui::Dummy(ImVec2(0.0f, 10.0f));
				//ImGui::SameLine(ImGui::GetItemRectSize().x + 10.0);
				ImGui::Text("%.3f ms", benchmarkAverage * 1000.0);

				mBenchmarkIdx[name] = mBenchmarkIdx[name] + 1 % s_max_benchmark_values;
			}
		}
	}
}
