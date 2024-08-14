#pragma once

#include <unordered_map>
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

		void deinitialize() override;

		void end_play() override;

		void start_benchmark(const std::string& name);
		void end_benchmark(const std::string& name);
		double get_benchmark_time(const std::string& name);

	private:

		std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> m_benchmark_start_point;
		std::unordered_map<std::string, double> m_benchmarks;

	};
}