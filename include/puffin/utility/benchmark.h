#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "nlohmann/json.hpp"
#include "puffin/core/timer.h"

namespace puffin::utility
{
    struct BenchmarkData
    {
        explicit BenchmarkData() = default;
        explicit BenchmarkData(std::string name) : name(std::move(name))
        {
        }

        std::string name;
        double timeElapsed = 0.0;
    };
    
    class Benchmark
    {
    public:

        explicit Benchmark();
        explicit Benchmark(std::string name);

        void Begin();
        void End();

        Benchmark* Begin(const std::string& name);
        Benchmark* End(const std::string& name);
        Benchmark* Get(const std::string& name);

        [[nodiscard]] const BenchmarkData& GetData() const;
        [[nodiscard]] const std::unordered_map<std::string, Benchmark>& GetBenchmarks() const;

        void ToJson(nlohmann::json& json) const;

    protected:

        BenchmarkData mBenchmarkData;
        core::Timer mTimer;

        std::unordered_map<std::string, Benchmark> mBenchmarks;
        
    };

    class BenchmarkManager : public Benchmark
    {
    public:

        static BenchmarkManager* Get();
        static void Destroy();

        Benchmark* Begin(const std::string& name);
        Benchmark* End(const std::string& name);
        Benchmark* Get(const std::string& name);
        void Clear();

        [[nodiscard]] const std::unordered_map<std::string, Benchmark>& GetBenchmarks() const;

        void ToJson(nlohmann::json& json) const;

    private:

        explicit BenchmarkManager() = default;

        static BenchmarkManager* sBenchmarkManager;

        std::unordered_map<std::string, Benchmark> mBenchmarks;
    };
}
