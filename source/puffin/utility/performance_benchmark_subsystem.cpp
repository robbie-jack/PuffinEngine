#include "puffin/utility/performance_benchmark_subsystem.h"

#include <cassert>

namespace puffin::utility
{
	PerformanceBenchmarkSubsystem::PerformanceBenchmarkSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
	}

	void PerformanceBenchmarkSubsystem::deinitialize()
	{
		m_benchmarks.clear();
		m_benchmark_start_point.clear();
	}

	void PerformanceBenchmarkSubsystem::end_play()
	{
		m_benchmarks.clear();
		m_benchmark_start_point.clear();
	}

	void PerformanceBenchmarkSubsystem::start_benchmark(const std::string& name)
	{
		if (m_benchmark_start_point.find(name) == m_benchmark_start_point.end())
		{
			m_benchmark_start_point.emplace(name, std::chrono::time_point<std::chrono::high_resolution_clock>());
		}

		m_benchmark_start_point[name] = std::chrono::high_resolution_clock::now();
	} 

	void PerformanceBenchmarkSubsystem::end_benchmark(const std::string& name)
	{
		assert(m_benchmark_start_point.find(name) != m_benchmark_start_point.end() && "PerformanceBenchmarkSubsystem::end_benchmark - no matching call to start_benchmark found");

		const auto benchmark_end_point = std::chrono::high_resolution_clock::now();

		const auto duration = benchmark_end_point - m_benchmark_start_point.at(name);

		if (m_benchmarks.find(name) == m_benchmarks.end())
		{
			m_benchmarks.emplace(name, 0.0);
		}

		using duration_milliseconds = std::chrono::duration<double, std::chrono::milliseconds::period>;
		m_benchmarks[name] = duration_milliseconds(duration).count();

		m_benchmark_start_point.erase(name);
	}

	double PerformanceBenchmarkSubsystem::get_benchmark_time(const std::string& name)
	{
		if (m_benchmarks.find(name) == m_benchmarks.end())
		{
			return 0.0;
		}

		return m_benchmarks.at(name);
	}
}
