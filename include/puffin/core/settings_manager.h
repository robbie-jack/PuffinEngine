#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "nlohmann/json.hpp"

#include "puffin/core/engine_subsystem.h"
#include "puffin/core/signal_subsystem.h"

namespace fs = std::filesystem;

namespace puffin::core
{
	class SettingsManager : public EngineSubsystem
    {
    public:

        explicit SettingsManager(const std::shared_ptr<core::Engine>& engine) : EngineSubsystem(engine) {}

        ~SettingsManager() override = default;

        template<typename T>
        void set(const std::string& name, const T& t)
        {
            m_json[name] = t;

            auto signal_subsystem = m_engine->get_engine_subsystem<SignalSubsystem>();

            if (!signal_subsystem->get_signal<T>(name))
            {
                signal_subsystem->create_signal<T>(name);
            }

            signal_subsystem->emit(name, t);
        }

        template<typename T>
        T get(const std::string& name) const
        {
            return m_json.at(name);
        }

        void load(const fs::path& path)
        {
            if (!fs::exists(path))
                return;

            std::ifstream is(path.string());
            is >> m_json;

            is.close();
        }

        void save(const fs::path& path)
        {
            if (!exists(path.parent_path()))
                create_directory(path.parent_path());

            std::ofstream os(path.string());

            os << std::setw(4) << m_json << std::endl;

            os.close();
        }

    private:

        nlohmann::json m_json;

    };
}
