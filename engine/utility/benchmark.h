#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "nlohmann/json.hpp"
#include "core/timer.h"

namespace puffin::utility
{
    struct BenchmarkData
    {
        explicit BenchmarkData() = default;
        explicit BenchmarkData(std::string_view name);

        std::string_view name;
        double timeElapsed = 0.0;
    };
    
    class Benchmark
    {
    public:

        explicit Benchmark();
        explicit Benchmark(std::string_view name);

        void Begin();
        void End();

        Benchmark* Begin(const std::string_view& name);
        Benchmark* End(const std::string_view& name);
        Benchmark* Get(const std::string_view& name);

        [[nodiscard]] const BenchmarkData& GetData() const;
        [[nodiscard]] const std::unordered_map<std::string_view, Benchmark>& GetBenchmarks() const;

        void ToJson(nlohmann::json& json) const;

    protected:

        BenchmarkData m_benchmarkData;
        core::Timer m_timer;

        std::unordered_map<std::string_view, Benchmark> m_benchmarks;
        
    };

    class BenchmarkManager
    {
    public:

        static BenchmarkManager* Get();
        static void Destroy();

        Benchmark* Begin(const std::string_view& name);
        Benchmark* End(const std::string_view& name);
        Benchmark* Get(const std::string_view& name);
        void Clear();

        [[nodiscard]] const std::unordered_map<std::string_view, Benchmark>& GetBenchmarks() const;

        void ToJson(nlohmann::json& json) const;

    private:

        explicit BenchmarkManager() = default;

        static BenchmarkManager* s_benchmarkManager;

        std::unordered_map<std::string_view, Benchmark> m_benchmarks;
    };
}
