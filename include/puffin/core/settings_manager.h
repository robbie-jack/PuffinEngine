#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "nlohmann/json.hpp"
#include "puffin/core/signal_subsystem.h"

namespace fs = std::filesystem;

namespace puffin
{
    class SettingsManager
    {
        static SettingsManager* s_instance;

        SettingsManager() = default;

    public:

        static SettingsManager* get()
        {
            if (!SettingsManager::s_instance)
                SettingsManager::s_instance = new SettingsManager();

            return SettingsManager::s_instance;
        }

        static void clear()
        {
            delete SettingsManager::s_instance;
            SettingsManager::s_instance = nullptr;
        }

        ~SettingsManager() = default;

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
            std::ofstream os(path.string());

            os << std::setw(4) << m_json << std::endl;

            os.close();
        }

    private:

        std::shared_ptr<core::SignalSubsystem> m_signal_subsystem;
        nlohmann::json m_json;

    };
}
