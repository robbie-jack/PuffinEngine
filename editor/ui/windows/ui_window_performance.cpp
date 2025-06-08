#include "ui/windows/ui_window_performance.h"

#include <imgui_internal.h>
#include <thread>
#include <math.h>
#include <stdio.h> 

#include "imgui.h"
#include "utility/benchmark.h"

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

					auto* benchmarkManager = utility::BenchmarkManager::Get();

					DrawBenchmark(benchmarkManager->Get("Input"));
					DrawBenchmark(benchmarkManager->Get("WaitForLastPresentationAndSample"));
					DrawBenchmark(benchmarkManager->Get("Idle"));
					DrawBenchmark(benchmarkManager->Get("EngineUpdate"));
					DrawBenchmark(benchmarkManager->Get("FixedUpdate"));
					DrawBenchmark(benchmarkManager->Get("Update"));
					DrawBenchmark(benchmarkManager->Get("Render"));
				}

				End();
			}
		}

		void UIWindowPerformance::DrawBenchmark(const utility::Benchmark* benchmark)
		{
			if (benchmark)
			{
				const std::string& benchmarkName = benchmark->GetData().name;
				double benchmarkTimeElapsed = benchmark->GetData().timeElapsed;
			
				if (mBenchmarkIdx.find(benchmarkName) == mBenchmarkIdx.end())
				{
					mBenchmarkIdx.emplace(benchmarkName, 0);

					mBenchmarkValues.emplace(benchmarkName, std::vector<double>());
					mBenchmarkValues.at(benchmarkName).reserve(s_max_benchmark_values);
				}

				const int benchmarkIdx = mBenchmarkIdx.at(benchmarkName) % s_max_benchmark_values;

				auto& benchmarkVector = mBenchmarkValues.at(benchmarkName);
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

				ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanAvailWidth;

				bool hasChildBenchmarks = false;
				if (!benchmark->GetBenchmarks().empty())
				{
					hasChildBenchmarks = true;
				}

				if (!hasChildBenchmarks)
				{
					treeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				}

				bool benchmarkOpen = ImGui::TreeNodeEx(benchmarkName.c_str(), treeFlags, "%s - %.3f ms",
					benchmarkName.c_str(), benchmarkAverage * 1000.0);

				if (benchmarkOpen && hasChildBenchmarks)
				{
					for (const auto& [benchmarkName, benchmarkChild] : benchmark->GetBenchmarks())
					{
						DrawBenchmark(&benchmarkChild);
					}

					ImGui::TreePop();
				}

				mBenchmarkIdx[benchmarkName] = mBenchmarkIdx[benchmarkName] + 1 % s_max_benchmark_values;
			}
		}
	}
}
