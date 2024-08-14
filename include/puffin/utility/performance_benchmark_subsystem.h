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

		void deinitialize() override;

		void end_play() override;

		void start_benchmark(const std::string& name);
		void start_benchmark_category(const std::string& name, const std::string& category);

		void end_benchmark(const std::string& name);
		void end_benchmark_category(const std::string& name, const std::string& category);

		double get_benchmark_time(const std::string& name);
		double get_benchmark_time_category(const std::string& name, const std::string& category);

		const std::unordered_set<std::string>& get_category(const std::string& category);

	private:

		using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
		using time_point_map = std::unordered_map<std::string, time_point>;
		using benchmark_map = std::unordered_map<std::string, double>;
		using string_set = std::unordered_set<std::string>;

		time_point_map m_benchmark_start_point;
		benchmark_map m_benchmarks;

		std::unordered_map<std::string, time_point_map> m_category_benchmark_start_point;
		std::unordered_map<std::string, benchmark_map> m_category_benchmarks;
		std::unordered_map<std::string, string_set> m_category_names;

	};
}