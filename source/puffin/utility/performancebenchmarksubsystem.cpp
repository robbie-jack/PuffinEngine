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
		mBenchmarks.clear();
	}

	void PerformanceBenchmarkSubsystem::EndPlay()
	{
		mBenchmarks.clear();

		mCategoryBenchmarks.clear();
		mCategoryNames.clear();
	}

	void PerformanceBenchmarkSubsystem::StartBenchmark(const std::string& name)
	{
		const auto startPoint = std::chrono::high_resolution_clock::now();

		if (mBenchmarks.find(name) == mBenchmarks.end())
		{
			mBenchmarks.emplace(name, core::Timer());
		}

		mBenchmarks[name].Start();
	}

	void PerformanceBenchmarkSubsystem::StartBenchmarkCategory(const std::string& name, const std::string& categoryName)
	{
		const auto startPoint = std::chrono::high_resolution_clock::now();

		if (mCategoryBenchmarks.find(categoryName) == mCategoryBenchmarks.end())
		{
			mCategoryBenchmarks.emplace(categoryName, BenchmarkTimerMap());
			mCategoryNames.emplace(categoryName, StringSet());
		}

		auto& categoryBenchmarkMap = mCategoryBenchmarks.at(categoryName);

		if (categoryBenchmarkMap.find(name) == categoryBenchmarkMap.end())
		{
			categoryBenchmarkMap.emplace(name, core::Timer());
			mCategoryNames[categoryName].emplace(name);
		}

		categoryBenchmarkMap[name].Start();
	}

	void PerformanceBenchmarkSubsystem::EndBenchmark(const std::string& name)
	{
		assert(mBenchmarks.find(name) != mBenchmarks.end() && "PerformanceBenchmarkSubsystem::EndBenchmark - No matching call to StartBenchmark found");

		mBenchmarks[name].End();
	}

	void PerformanceBenchmarkSubsystem::EndBenchmarkCategory(const std::string& name, const std::string& categoryName)
	{
		assert(mCategoryBenchmarks.find(categoryName) != mCategoryBenchmarks.end() && "PerformanceBenchmarkSubsystem::EndBenchmarkCategory - No matching category found");

		auto& categoryBenchmarkMap = mCategoryBenchmarks.at(categoryName);

		assert(categoryBenchmarkMap.find(name) != categoryBenchmarkMap.end() && "PerformanceBenchmarkSubsystem::EndBenchmarkCategory - No matching call to StartBenchmarkCategory found");

		categoryBenchmarkMap[name].End();
	}

	double PerformanceBenchmarkSubsystem::GetBenchmarkTime(const std::string& name)
	{
		if (mBenchmarks.find(name) == mBenchmarks.end())
		{
			return 0.0;
		}

		return mBenchmarks.at(name).GetElapsedTime();
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

		return categoryBenchmarkMap.at(name).GetElapsedTime();
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
