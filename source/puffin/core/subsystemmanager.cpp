#include "puffin/core/subsystemmanager.h"

#include "puffin/core/engine.h"

namespace puffin::core
{
	SubsystemManager::SubsystemManager(const std::shared_ptr<Engine>& engine) : mEngine(engine)
	{
	}

	std::vector<Subsystem*>& SubsystemManager::GetSubsystems()
	{
		return mInitializedEngineSubsystems;
	}

	std::vector<Subsystem*>& SubsystemManager::GetGameplaySubsystems()
	{
		return mInitializedGameplaySubsystems;
	}

	Subsystem* SubsystemManager::GetInputSubsystem() const
	{
		assert(mInputSubsystem != nullptr && "SubsystemManager::GetInputSubsystem() - Attempting to get input subsystem while it is invalid");

		return mInputSubsystem;
	}

	Subsystem* SubsystemManager::GetRenderSubsystem() const
	{
		assert(mRenderSubsystem != nullptr && "SubsystemManager::GetRenderSubsystem() - Attempting to get render subsystem while it is invalid");

		return mRenderSubsystem;
	}

	void SubsystemManager::CreateAndInitializeEngineSubsystems()
	{
		for (const auto& typeName : mEngineSubsystemNames)
		{
			auto subsystem = CreateAndInitializeSubsystemInternal(typeName);

			if (subsystem->GetType() == SubsystemType::Input)
			{
				assert(mInputSubsystem == nullptr && "SubsystemManager::CreateAndInitializeEngineSubsystems - Attempting to initialize a second input subsystem");

				mInputSubsystem = subsystem;
			}

			if (subsystem->GetType() == SubsystemType::Render)
			{
				assert(mRenderSubsystem == nullptr && "SubsystemManager::CreateAndInitializeEngineSubsystems - Attempting to initialize a second render subsystem");

				mRenderSubsystem = subsystem;
			}
		}
	}

	void SubsystemManager::CreateAndInitializeGameplaySubsystems()
	{
		for (const auto& typeName : mGameplaySubsystemNames)
		{
			auto Subsystem = CreateAndInitializeSubsystemInternal(typeName);
		}
	}

	void SubsystemManager::DestroyEngineSubsystems()
	{
		for (auto it = mInitializedEngineSubsystems.rbegin(); it != mInitializedEngineSubsystems.rend(); ++it)
		{
			(*it)->Deinitialize();
			delete *it;
		}

		mInitializedEngineSubsystems.clear();

		for (const auto& typeName : mEngineSubsystemNames)
		{
			mInitializedSubsystems.erase(typeName);
			mSubsystems.erase(typeName);
		}
	}

	void SubsystemManager::DestroyGameplaySubsystems()
	{
		for (auto it = mInitializedGameplaySubsystems.rbegin(); it != mInitializedGameplaySubsystems.rend(); ++it)
		{
			(*it)->Deinitialize();
			delete* it;
		}

		mInitializedGameplaySubsystems.clear();

		for (const auto& typeName : mGameplaySubsystemNames)
		{
			mInitializedSubsystems.erase(typeName);
			mSubsystems.erase(typeName);
		}
	}

	bool SubsystemManager::IsEditorType(SubsystemType type)
	{
		if (type == SubsystemType::Gameplay)
			return false;

		return true;
	}

	bool SubsystemManager::IsGameplayType(SubsystemType type)
	{
		if (type == SubsystemType::Gameplay)
			return true;

		return false;
	}

	Subsystem* SubsystemManager::CreateSubsystemInternal(const char* typeName)
	{
		// Return if subsystem of this type is already created
		if (mSubsystems.find(typeName) != mSubsystems.end())
		{
			return mSubsystems.at(typeName);
		}

		assert(mSubsystemFactories.find(typeName) != mSubsystemFactories.end() && "SubsystemManager::create_subsystem_internal() - Attempting to create subsystem that wasn't registered");

		const auto& subsystemFactory = mSubsystemFactories.at(typeName);

		auto subsystem = subsystemFactory->Create(mEngine);
		mSubsystems.emplace(typeName, subsystem);

		return subsystem;
	}

	void SubsystemManager::InitializeSubsystemInternal(const char* typeName)
	{
		if (mInitializedSubsystems.find(typeName) != mInitializedSubsystems.end())
		{
			return;
		}

		assert(mSubsystemFactories.find(typeName) != mSubsystemFactories.end() && "SubsystemManager::initialize_subsystem_internal() - Attempting to initialize subsystem that wasn't registered");
		assert(mSubsystems.find(typeName) != mSubsystems.end() && "SubsystemManager::initialize_subsystem_internal() - Attempting to initialize subsystem that hasn't been created yet");

		auto subsystem = mSubsystems.at(typeName);

		if (subsystem->GetType() == SubsystemType::Editor && !mEngine->GetShouldRenderEditorUI())
		{
			return;
		}

		subsystem->Initialize(this);

		mInitializedSubsystems.emplace(typeName, subsystem);

		if (IsEditorType(subsystem->GetType()))
		{
			mInitializedEngineSubsystems.push_back(subsystem);
		}
		else
		{
			mInitializedGameplaySubsystems.push_back(subsystem);
		}
	}

	
	Subsystem* SubsystemManager::CreateAndInitializeSubsystemInternal(const char* typeName)
	{
		auto subsystem = CreateSubsystemInternal(typeName);

		InitializeSubsystemInternal(typeName);

		return subsystem;
	}
}
