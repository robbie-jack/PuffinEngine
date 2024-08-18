#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "engine.h"
#include "nlohmann/json.hpp"

#include "puffin/core/subsystem.h"
#include "puffin/core/signalsubsystem.h"

namespace fs = std::filesystem;

namespace puffin::core
{
	class SettingsManager : public Subsystem
    {
    public:

        explicit SettingsManager(const std::shared_ptr<core::Engine>& engine);

        ~SettingsManager() override = default;

        void Initialize(core::SubsystemManager* subsystemManager) override;

        template<typename T>
        void Set(const std::string& name, const T& t)
        {
            m_json[name] = t;

            auto signal_subsystem = mEngine->GetSubsystem<SignalSubsystem>();

            if (!signal_subsystem->GetSignal<T>(name))
            {
                signal_subsystem->CreateSignal<T>(name);
            }

            signal_subsystem->Emit(name, t);
        }

        template<typename T>
        T Get(const std::string& name) const
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
