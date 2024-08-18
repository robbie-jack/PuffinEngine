#include "puffin/utility/performancebenchmarksubsystem.h"

#include <cassert>

namespace puffin::utility
{
	PerformanceBenchmarkSubsystem::PerformanceBenchmarkSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "PerformanceBenchmarkSubsystem";
	}

	void PerformanceBenchmarkSubsystem::Deinitialize()
	{
		mBenchmarkStartPoint.clear();
		mBenchmarks.clear();
	}

	void PerformanceBenchmarkSubsystem::EndPlay()
	{
		mBenchmarkStartPoint.clear();
		mBenchmarks.clear();

		mCategoryBenchmarkStartPoint.clear();
		mCategoryBenchmarks.clear();
		mCategoryNames.clear();
	}

	void PerformanceBenchmarkSubsystem::StartBenchmark(const std::string& name)
	{
		const auto startPoint = std::chrono::high_resolution_clock::now();

		if (mBenchmarkStartPoint.find(name) == mBenchmarkStartPoint.end())
		{
			mBenchmarkStartPoint.emplace(name, TimePoint());
		}

		mBenchmarkStartPoint[name] = startPoint;
	}

	void PerformanceBenchmarkSubsystem::StartBenchmarkCategory(const std::string& name, const std::string& categoryName)
	{
		const auto startPoint = std::chrono::high_resolution_clock::now();

		if (mCategoryBenchmarkStartPoint.find(categoryName) == mCategoryBenchmarkStartPoint.end())
		{
			mCategoryBenchmarkStartPoint.emplace(categoryName, TimePointMap());
			mCategoryBenchmarks.emplace(categoryName, BenchmarkMap());
			mCategoryNames.emplace(categoryName, StringSet());
		}

		auto& categoryTimePointMap = mCategoryBenchmarkStartPoint.at(categoryName);

		if (categoryTimePointMap.find(name) == categoryTimePointMap.end())
		{
			categoryTimePointMap.emplace(name, TimePoint());
			mCategoryNames[categoryName].emplace(name);
		}

		categoryTimePointMap[name] = startPoint;
	}

	void PerformanceBenchmarkSubsystem::EndBenchmark(const std::string& name)
	{
		assert(mBenchmarkStartPoint.find(name) != mBenchmarkStartPoint.end() && "PerformanceBenchmarkSubsystem::EndBenchmark - No matching call to start_benchmark found");

		const auto benchmarkEndPoint = std::chrono::high_resolution_clock::now();

		const auto duration = benchmarkEndPoint - mBenchmarkStartPoint.at(name);

		if (mBenchmarks.find(name) == mBenchmarks.end())
		{
			mBenchmarks.emplace(name, 0.0);
		}

		using DurationMilliseconds = std::chrono::duration<double, std::chrono::milliseconds::period>;
		mBenchmarks[name] = DurationMilliseconds(duration).count();
	}

	void PerformanceBenchmarkSubsystem::EndBenchmarkCategory(const std::string& name, const std::string& categoryName)
	{
		assert(mCategoryBenchmarkStartPoint.find(categoryName) != mCategoryBenchmarkStartPoint.end() && "PerformanceBenchmarkSubsystem::EndBenchmarkCategory - No matching category found");

		auto& categoryTimePointMap = mCategoryBenchmarkStartPoint.at(categoryName);

		assert(categoryTimePointMap.find(name) != categoryTimePointMap.end() && "PerformanceBenchmarkSubsystem::EndBenchmarkCategory - No matching call to start_benchmark_category found");

		const auto benchmarkEndPoint = std::chrono::high_resolution_clock::now();

		const auto duration = benchmarkEndPoint - categoryTimePointMap.at(name);

		auto& categoryBenchmarkMap = mCategoryBenchmarks.at(categoryName);
		if (categoryBenchmarkMap.find(name) == categoryBenchmarkMap.end())
		{
			categoryBenchmarkMap.emplace(name, 0.0);
		}

		using DurationMilliseconds = std::chrono::duration<double, std::chrono::milliseconds::period>;
		categoryBenchmarkMap[name] = DurationMilliseconds(duration).count();
	}

	double PerformanceBenchmarkSubsystem::GetBenchmarkTime(const std::string& name)
	{
		if (mBenchmarks.find(name) == mBenchmarks.end())
		{
			return 0.0;
		}

		return mBenchmarks.at(name);
	}

	double PerformanceBenchmarkSubsystem::GetBenchmarkTimeCategory(const std::string& name,
		const std::string& categoryName)
	{
		if (mCategoryBenchmarks.find(categoryName) == mCategoryBenchmarks.end())
		{
			return 0.0;
		}

		auto& categoryBenchmarkMap = mCategoryBenchmarks.at(categoryName);
		if (categoryBenchmarkMap.find(name) == categoryBenchmarkMap.end())
		{
			return 0.0;
		}

		return categoryBenchmarkMap[name];
	}

	const std::unordered_set<std::string>& PerformanceBenchmarkSubsystem::GetCategory(const std::string& categoryName)
	{
		if (mCategoryNames.find(categoryName) == mCategoryNames.end())
		{
			mCategoryNames.emplace(categoryName, StringSet());
		}

		return mCategoryNames.at(categoryName);
	}
}
