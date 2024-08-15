#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "nlohmann/json.hpp"

#include "puffin/core/subsystem.h"
#include "puffin/core/signal_subsystem.h"

namespace fs = std::filesystem;

namespace puffin::core
{
	class SettingsManager : public Subsystem
    {
    public:

        explicit SettingsManager(const std::shared_ptr<core::Engine>& engine);

        ~SettingsManager() override = default;

        void initialize(core::SubsystemManager* subsystem_manager) override;

        template<typename T>
        void set(const std::string& name, const T& t)
        {
            m_json[name] = t;

            auto signal_subsystem = m_engine->get_subsystem<SignalSubsystem>();

            if (!signal_subsystem->get_signal<T>(name))
            {
                signal_subsystem->create_signal<T>(name);
            }

            signal_subsystem->emit(name, t);
        }

        template<typename T>
        T get(const std::string& name) const
        {
            if (!m_json.contains(name))
            {
            	return T{};
            }

            return m_json.at(name);
        }

        void load(const fs::path& path);
        void save(const fs::path& path);

    private:

        nlohmann::json m_json;

        void default_settings();

    };
}
