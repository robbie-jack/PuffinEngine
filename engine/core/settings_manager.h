#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "engine.h"
#include "toml++/toml.hpp"

#include "core/subsystem.h"
#include "core/signal_subsystem.h"

namespace fs = std::filesystem;

namespace puffin::core
{
    class SettingsCategory
    {
    public:

    	SettingsCategory() = default;
    	SettingsCategory(std::shared_ptr<Engine> engine, std::string name);
    	~SettingsCategory() = default;

        template<typename T>
        std::optional<T> Get(const std::string& name)
        {
            return mData.at(name).value<T>();
        }

        template<typename T>
        void Set(const std::string& name, const T& value)
        {
        	mData.insert_or_assign(name, value);
        	
	        const auto signalSubsystem = mEngine->GetSubsystem<SignalSubsystem>();

        	auto signal = signalSubsystem->GetOrCreateSignal(mName + "_" + name);
        	signal->Emit();
        }

        void SetData(const toml::table& data);
        [[nodiscard]] const toml::table& GetData() const;

    private:
    	
    	std::shared_ptr<Engine> mEngine = nullptr;
    	std::string mName;
        toml::table mData;
        
    };
    
	class SettingsManager : public Subsystem
    {
    public:

        explicit SettingsManager(const std::shared_ptr<Engine>& engine);

        ~SettingsManager() override = default;

        void Initialize(core::SubsystemManager* subsystemManager) override;
	    
        SettingsCategory& GetCategory(const std::string& name);

	    template<typename T>
	    std::optional<T> Get(const std::string& categoryName, const std::string& name)
	    {
	        return GetCategory(categoryName).Get<T>(name);
	    }

	    template<typename T>
	    void Set(const std::string& categoryName, const std::string& name, const T& value)
	    {
	        GetCategory(categoryName).Set<T>(name, value);
	    }

        void Load(const fs::path& path);
        void Save(const fs::path& path);

    private:
	    
	    std::unordered_map<std::string, SettingsCategory> mCategories;

        void DefaultSettings();

    };
}
