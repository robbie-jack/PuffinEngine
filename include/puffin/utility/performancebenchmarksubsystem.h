#pragma once

#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <string>

#include "puffin/core/subsystem.h"
#include "puffin/core/timer.h"

namespace puffin::utility
{
	/*
	 * Subsystem for performing basic benchmarking of Puffin code
	 */
	class PerformanceBenchmarkSubsystem : public core::Subsystem
	{
	public:

		explicit PerformanceBenchmarkSubsystem(const std::shared_ptr<core::Engine>& engine);
		~PerformanceBenchmarkSubsystem() override = default;

		void Deinitialize() override;

		void EndPlay() override;

		void StartBenchmark(const std::string& name);
		void StartBenchmarkCategory(const std::string& name, const std::string& categoryName);

		void EndBenchmark(const std::string& name);
		void EndBenchmarkCategory(const std::string& name, const std::string& categoryName);

		double GetBenchmarkTime(const std::string& name);
		double GetBenchmarkTimeCategory(const std::string& name, const std::string& categoryName);

		const std::unordered_set<std::string>& GetCategory(const std::string& categoryName);

	private:

		using BenchmarkTimerMap = std::unordered_map<std::string, core::Timer>;
		using StringSet = std::unordered_set<std::string>;

		BenchmarkTimerMap mBenchmarks;

		std::unordered_map<std::string, BenchmarkTimerMap> mCategoryBenchmarks;
		std::unordered_map<std::string, StringSet> mCategoryNames;

	};
}