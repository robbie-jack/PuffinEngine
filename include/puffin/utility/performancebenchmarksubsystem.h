#pragma once

#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <string>

#include "puffin/core/subsystem.h"

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

		using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
		using TimePointMap = std::unordered_map<std::string, TimePoint>;
		using BenchmarkMap = std::unordered_map<std::string, double>;
		using StringSet = std::unordered_set<std::string>;

		TimePointMap mBenchmarkStartPoint;
		BenchmarkMap mBenchmarks;

		std::unordered_map<std::string, TimePointMap> mCategoryBenchmarkStartPoint;
		std::unordered_map<std::string, BenchmarkMap> mCategoryBenchmarks;
		std::unordered_map<std::string, StringSet> mCategoryNames;

	};
}