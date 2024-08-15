#include "puffin/utility/performance_benchmark_subsystem.h"

#include <cassert>

namespace puffin::utility
{
	PerformanceBenchmarkSubsystem::PerformanceBenchmarkSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		m_name = "PerformanceBenchmarkSubsystem";
	}

	void PerformanceBenchmarkSubsystem::deinitialize()
	{
		m_benchmark_start_point.clear();
		m_benchmarks.clear();
	}

	void PerformanceBenchmarkSubsystem::end_play()
	{
		m_benchmark_start_point.clear();
		m_benchmarks.clear();

		m_category_benchmark_start_point.clear();
		m_category_benchmarks.clear();
		m_category_names.clear();
	}

	void PerformanceBenchmarkSubsystem::start_benchmark(const std::string& name)
	{
		auto start_point = std::chrono::high_resolution_clock::now();

		if (m_benchmark_start_point.find(name) == m_benchmark_start_point.end())
		{
			m_benchmark_start_point.emplace(name, time_point());
		}

		m_benchmark_start_point[name] = start_point;
	}

	void PerformanceBenchmarkSubsystem::start_benchmark_category(const std::string& name, const std::string& category)
	{
		auto start_point = std::chrono::high_resolution_clock::now();

		if (m_category_benchmark_start_point.find(category) == m_category_benchmark_start_point.end())
		{
			m_category_benchmark_start_point.emplace(category, time_point_map());
			m_category_benchmarks.emplace(category, benchmark_map());
			m_category_names.emplace(category, string_set());
		}

		auto& category_time_point_map = m_category_benchmark_start_point.at(category);

		if (category_time_point_map.find(name) == category_time_point_map.end())
		{
			category_time_point_map.emplace(name, time_point());
			m_category_names[category].emplace(name);
		}

		category_time_point_map[name] = start_point;
	}

	void PerformanceBenchmarkSubsystem::end_benchmark(const std::string& name)
	{
		assert(m_benchmark_start_point.find(name) != m_benchmark_start_point.end() && "PerformanceBenchmarkSubsystem::end_benchmark - No matching call to start_benchmark found");

		const auto benchmark_end_point = std::chrono::high_resolution_clock::now();

		const auto duration = benchmark_end_point - m_benchmark_start_point.at(name);

		if (m_benchmarks.find(name) == m_benchmarks.end())
		{
			m_benchmarks.emplace(name, 0.0);
		}

		using duration_milliseconds = std::chrono::duration<double, std::chrono::milliseconds::period>;
		m_benchmarks[name] = duration_milliseconds(duration).count();
	}

	void PerformanceBenchmarkSubsystem::end_benchmark_category(const std::string& name, const std::string& category)
	{
		assert(m_category_benchmark_start_point.find(category) != m_category_benchmark_start_point.end() && "PerformanceBenchmarkSubsystem::end_benchmark_category - No matching category found");

		auto& category_time_point_map = m_category_benchmark_start_point.at(category);

		assert(category_time_point_map.find(name) != category_time_point_map.end() && "PerformanceBenchmarkSubsystem::end_benchmark_category - No matching call to start_benchmark_category found");

		const auto benchmark_end_point = std::chrono::high_resolution_clock::now();

		const auto duration = benchmark_end_point - category_time_point_map.at(name);

		auto& category_benchmark_map = m_category_benchmarks.at(category);
		if (category_benchmark_map.find(name) == category_benchmark_map.end())
		{
			category_benchmark_map.emplace(name, 0.0);
		}

		using duration_milliseconds = std::chrono::duration<double, std::chrono::milliseconds::period>;
		category_benchmark_map[name] = duration_milliseconds(duration).count();
	}

	double PerformanceBenchmarkSubsystem::get_benchmark_time(const std::string& name)
	{
		if (m_benchmarks.find(name) == m_benchmarks.end())
		{
			return 0.0;
		}

		return m_benchmarks.at(name);
	}

	double PerformanceBenchmarkSubsystem::get_benchmark_time_category(const std::string& name,
		const std::string& category)
	{
		if (m_category_benchmarks.find(category) == m_category_benchmarks.end())
		{
			return 0.0;
		}

		auto& category_benchmark_map = m_category_benchmarks.at(category);
		if (category_benchmark_map.find(name) == category_benchmark_map.end())
		{
			return 0.0;
		}

		return category_benchmark_map[name];
	}

	const std::unordered_set<std::string>& PerformanceBenchmarkSubsystem::get_category(const std::string& category)
	{
		if (m_category_names.find(category) == m_category_names.end())
		{
			m_category_names.emplace(category, string_set());
		}

		return m_category_names.at(category);
	}
}
