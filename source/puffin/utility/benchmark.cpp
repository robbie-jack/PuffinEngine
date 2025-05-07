#include "puffin/utility/benchmark.h"

namespace puffin::utility
{
    BenchmarkManager* BenchmarkManager::sBenchmarkManager = nullptr;

    Benchmark::Benchmark()
    {
    }

    Benchmark::Benchmark(std::string name) : mBenchmarkData(std::move(name))
    {
        
    }

    void Benchmark::Begin()
    {
        mTimer.Start();
    }

    void Benchmark::End()
    {
        mTimer.End();

        mBenchmarkData.timeElapsed = mTimer.GetElapsedTime();
    }

    Benchmark* Benchmark::Begin(const std::string& name)
    {
        if (mBenchmarks.find(name) == mBenchmarks.end())
        {
            mBenchmarks.emplace(name, Benchmark(name));
        }

        mBenchmarks[name].Begin();
        return &mBenchmarks[name];
    }

    Benchmark* Benchmark::End(const std::string& name)
    {
        if (mBenchmarks.find(name) == mBenchmarks.end())
            return nullptr;

        mBenchmarks[name].End();
        return &mBenchmarks[name];
    }

    Benchmark* Benchmark::Get(const std::string& name)
    {
        if (mBenchmarks.find(name) == mBenchmarks.end())
            return nullptr;

        return &mBenchmarks[name];
    }

    const BenchmarkData& Benchmark::GetData() const
    {
        return mBenchmarkData;
    }

    const std::unordered_map<std::string, Benchmark>& Benchmark::GetBenchmarks() const
    {
        return mBenchmarks;
    }

    void Benchmark::ToJson(nlohmann::json& json) const
    {
        json["name"] = mBenchmarkData.name;
        json["timeElapsed"] = mBenchmarkData.timeElapsed;

        std::vector<nlohmann::json> benchmarks;
        for (const auto& [name, benchmark] : mBenchmarks)
        {
            nlohmann::json benchmarkJson;
            benchmark.ToJson(benchmarkJson);

            benchmarks.push_back(benchmarkJson);
        }

        json["benchmarks"] = benchmarks;
    }

    BenchmarkManager* BenchmarkManager::Get()
    {
        if (!sBenchmarkManager)
        {
            sBenchmarkManager = new BenchmarkManager();
        }

        return sBenchmarkManager;
    }

    void BenchmarkManager::Destroy()
    {
        if (sBenchmarkManager)
        {
            delete sBenchmarkManager;
            sBenchmarkManager = nullptr;
        }
    }

    Benchmark* BenchmarkManager::Begin(const std::string& name)
    {
        if (mBenchmarks.find(name) == mBenchmarks.end())
        {
            mBenchmarks.emplace(name, Benchmark(name));
        }

        mBenchmarks[name].Begin();
        return &mBenchmarks[name];
    }

    Benchmark* BenchmarkManager::End(const std::string& name)
    {
        if (mBenchmarks.find(name) == mBenchmarks.end())
            return nullptr;

        mBenchmarks[name].End();
        return &mBenchmarks[name];
    }

    Benchmark* BenchmarkManager::Get(const std::string& name)
    {
        if (mBenchmarks.find(name) == mBenchmarks.end())
            return nullptr;

        return &mBenchmarks[name];
    }

    void BenchmarkManager::Clear()
    {
        mBenchmarks.clear();
    }

    const std::unordered_map<std::string, Benchmark>& BenchmarkManager::GetBenchmarks() const
    {
        return mBenchmarks;
    }

    void BenchmarkManager::ToJson(nlohmann::json& json) const
    {
        std::vector<nlohmann::json> benchmarks;
        for (const auto& [name, benchmark] : mBenchmarks)
        {
            nlohmann::json benchmarkJson;
            benchmark.ToJson(benchmarkJson);

            benchmarks.push_back(benchmarkJson);
        }

        json["benchmarks"] = benchmarks;
    }
}
