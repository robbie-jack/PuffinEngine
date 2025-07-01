#include "utility/benchmark.h"

namespace puffin::utility
{
    BenchmarkManager* BenchmarkManager::s_benchmarkManager = nullptr;

    BenchmarkData::BenchmarkData(std::string_view name): name(name)
    {
    }

    Benchmark::Benchmark() = default;

    Benchmark::Benchmark(std::string_view name) : m_benchmarkData(name)
    {
    }

    void Benchmark::Begin()
    {
        m_timer.Start();
    }

    void Benchmark::End()
    {
        m_timer.End();

        m_benchmarkData.timeElapsed = m_timer.GetElapsedTime();
    }

    Benchmark* Benchmark::Begin(const std::string_view& name)
    {
        if (m_benchmarks.find(name) == m_benchmarks.end())
        {
            m_benchmarks.emplace(name, Benchmark(name));
        }

        m_benchmarks[name].Begin();
        return &m_benchmarks[name];
    }

    Benchmark* Benchmark::End(const std::string_view& name)
    {
        if (m_benchmarks.find(name) == m_benchmarks.end())
            return nullptr;

        m_benchmarks[name].End();
        return &m_benchmarks[name];
    }

    Benchmark* Benchmark::Get(const std::string_view& name)
    {
        if (m_benchmarks.find(name) == m_benchmarks.end())
            return nullptr;

        return &m_benchmarks[name];
    }

    const BenchmarkData& Benchmark::GetData() const
    {
        return m_benchmarkData;
    }

    const std::unordered_map<std::string_view, Benchmark>& Benchmark::GetBenchmarks() const
    {
        return m_benchmarks;
    }

    void Benchmark::ToJson(nlohmann::json& json) const
    {
        json["name"] = m_benchmarkData.name;
        json["timeElapsed"] = m_benchmarkData.timeElapsed;

        std::vector<nlohmann::json> benchmarks;
        for (const auto& [name, benchmark] : m_benchmarks)
        {
            nlohmann::json benchmarkJson;
            benchmark.ToJson(benchmarkJson);

            benchmarks.push_back(benchmarkJson);
        }

        json["benchmarks"] = benchmarks;
    }

    BenchmarkManager* BenchmarkManager::Get()
    {
        if (!s_benchmarkManager)
        {
            s_benchmarkManager = new BenchmarkManager();
        }

        return s_benchmarkManager;
    }

    void BenchmarkManager::Destroy()
    {
        if (s_benchmarkManager)
        {
            delete s_benchmarkManager;
            s_benchmarkManager = nullptr;
        }
    }

    Benchmark* BenchmarkManager::Begin(const std::string_view& name)
    {
        if (m_benchmarks.find(name) == m_benchmarks.end())
        {
            m_benchmarks.emplace(name, Benchmark(name));
        }

        m_benchmarks[name].Begin();
        return &m_benchmarks[name];
    }

    Benchmark* BenchmarkManager::End(const std::string_view& name)
    {
        if (m_benchmarks.find(name) == m_benchmarks.end())
            return nullptr;

        m_benchmarks[name].End();
        return &m_benchmarks[name];
    }

    Benchmark* BenchmarkManager::Get(const std::string_view& name)
    {
        if (m_benchmarks.find(name) == m_benchmarks.end())
            return nullptr;

        return &m_benchmarks[name];
    }

    void BenchmarkManager::Clear()
    {
        m_benchmarks.clear();
    }

    const std::unordered_map<std::string_view, Benchmark>& BenchmarkManager::GetBenchmarks() const
    {
        return m_benchmarks;
    }

    void BenchmarkManager::ToJson(nlohmann::json& json) const
    {
        std::vector<nlohmann::json> benchmarks;
        for (const auto& [name, benchmark] : m_benchmarks)
        {
            nlohmann::json benchmarkJson;
            benchmark.ToJson(benchmarkJson);

            benchmarks.push_back(benchmarkJson);
        }

        json["benchmarks"] = benchmarks;
    }
}
