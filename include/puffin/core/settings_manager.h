#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "nlohmann/json.hpp"
#include "puffin/core/system.h"
#include "puffin/core/signal_subsystem.h"

namespace fs = std::filesystem;

namespace puffin::core
{
	class SettingsManager : public System
    {
    public:

        explicit SettingsManager(const std::shared_ptr<core::Engine>& engine) : System(engine) {};

        ~SettingsManager() override = default;

        void set_signal_subsystem(const std::shared_ptr<core::SignalSubsystem>& signal_subsystem)
        {
            m_signal_subsystem = signal_subsystem;
        }

        template<typename T>
        void set(const std::string& name, const T& t)
        {
            m_json[name] = t;

            if (!m_signal_subsystem->get_signal<T>(name))
            {
                m_signal_subsystem->create_signal<T>(name);
            }

            m_signal_subsystem->emit(name, t);
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

        std::shared_ptr<core::SignalSubsystem> m_signal_subsystem;
        nlohmann::json m_json;

    };
}
