#pragma once

#include <unordered_map>
#include <memory>
#include <cassert>

#include "core/subsystem.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class WindowSubsystem;
	}

	namespace input
	{
		class InputSubsystem;
	}
	namespace rendering
	{
		class RenderSubsystem;
	}
}

namespace puffin::core
{
	class ISubsystemFactory
	{
	public:

		virtual ~ISubsystemFactory() = default;

		[[nodiscard]] virtual Subsystem* Create(const std::shared_ptr<Engine>& engine) const = 0;

	};

	template<typename SubsystemT>
	class SubsystemFactory : public ISubsystemFactory
	{
	public:

		SubsystemFactory() {}
		~SubsystemFactory() override = default;

		[[nodiscard]] Subsystem* Create(const std::shared_ptr<Engine>& engine) const final
		{
			return new SubsystemT(engine);
		}
	};

	class SubsystemManager
	{
	public:

		explicit SubsystemManager(const std::shared_ptr<Engine>& engine);
		~SubsystemManager() { mEngine = nullptr; }

		template<typename T>
		void RegisterSubsystem()
		{
			const char* typeName = typeid(T).name();

			// Add new subsystem factory
			assert(mSubsystemFactories.find(typeName) == mSubsystemFactories.end() && "SubsystemManager::RegisterSubsystem() - Attempting to register subsystem more than once");

			mSubsystemFactories.emplace(typeName, new SubsystemFactory<T>());
			auto factory = mSubsystemFactories.at(typeName);

			// Create uninitialized instance of subsystem
			auto subsystem = factory->Create(mEngine);
			mSubsystems.emplace(typeName, subsystem);

			if (subsystem->GetType() == SubsystemType::Window)
			{
				assert(mRegisteredWindowSubsystem == false && "SubsystemManager::RegisterSubsystem - Attempting to register a second window subsystem");

				mRegisteredWindowSubsystem = true;
			}

			if (subsystem->GetType() == SubsystemType::Input)
			{
				assert(mRegisteredInputSubsystem == false && "SubsystemManager::RegisterSubsystem - Attempting to register a second input subsystem");

				mRegisteredInputSubsystem = true;
			}

			if (subsystem->GetType() == SubsystemType::Render)
			{
				assert(mRegisteredRenderSubsystem == false && "SubsystemManager::RegisterSubsystem - Attempting to register a second render subsystem");

				mRegisteredRenderSubsystem = true;
			}

			if (IsEditorType(subsystem->GetType()))
			{
				mEngineSubsystemNames.push_back(typeName);
			}
			else
			{
				mGameplaySubsystemNames.push_back(typeName);
			}

			subsystem->RegisterTypes();
		}

		template<typename T>
		T* GetSubsystem()
		{
			const char* typeName = typeid(T).name();

			assert(mInitializedSubsystems.find(typeName) != mInitializedSubsystems.end() && "SubsystemManager::GetSubsystem() - Attempting to get subsystem which has not been initialized");

			return static_cast<T*>(mInitializedSubsystems.at(typeName));
		}

		std::vector<Subsystem*>& GetEngineSubsystems();
		std::vector<Subsystem*>& GetGameplaySubsystems();

		[[nodiscard]] window::WindowSubsystem* GetWindowSubsystem() const;
		[[nodiscard]] input::InputSubsystem* GetInputSubsystem() const;
		[[nodiscard]] rendering::RenderSubsystem* GetRenderSubsystem() const;

		template<typename T>
		T* CreateAndInitializeSubsystem()
		{
			const char* typeName = typeid(T).name();
			auto subsystem = CreateSubsystemInternal(typeName);

			InitializeSubsystemInternal(typeName);

			return static_cast<T*>(subsystem);
		}

		void CreateAndInitializeEngineSubsystems();
		void CreateAndInitializeGameplaySubsystems();

		void DestroyEngineSubsystems();
		void DestroyGameplaySubsystems();

	private:

		static bool IsEditorType(SubsystemType type);
		static bool IsGameplayType(SubsystemType type);

		Subsystem* CreateSubsystemInternal(const char* typeName);
		void InitializeSubsystemInternal(const char* typeName);

		Subsystem* CreateAndInitializeSubsystemInternal(const char* typeName);

		std::shared_ptr<Engine> mEngine = nullptr;

		// Array of all subsystems, initialized and uninitialized
		std::unordered_map<const char*, Subsystem*> mSubsystems;

		// Array of all initialized subsystems
		std::unordered_map<const char*, Subsystem*> mInitializedSubsystems;

		std::unordered_map<const char*, ISubsystemFactory*> mSubsystemFactories;

		std::vector<const char*> mEngineSubsystemNames;
		std::vector<Subsystem*> mInitializedEngineSubsystems;

		std::vector<const char*> mGameplaySubsystemNames;
		std::vector<Subsystem*> mInitializedGameplaySubsystems;

		bool mRegisteredWindowSubsystem = false;
		window::WindowSubsystem* mWindowSubsystem = nullptr;

		bool mRegisteredInputSubsystem = false;
		input::InputSubsystem* mInputSubsystem = nullptr;

		bool mRegisteredRenderSubsystem = false;
		rendering::RenderSubsystem* mRenderSubsystem = nullptr;
	};
}
