#include "puffin/ecs/enttsubsystem.h"

namespace puffin::ecs
{
	EnTTSubsystem::EnTTSubsystem(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
	}

	EnTTSubsystem::~EnTTSubsystem()
	{
		mEngine = nullptr;
	}

	void EnTTSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		Subsystem::Initialize(subsystemManager);

		mRegistry = std::make_shared<entt::registry>();
	}

	void EnTTSubsystem::Deinitialize()
	{
		Subsystem::Deinitialize();

		mRegistry = nullptr;
	}

	void EnTTSubsystem::EndPlay()
	{
		mRegistry->clear();

		mIDToEntity.clear();
		mShouldBeSerialized.clear();
		mEntityToID.clear();
	}

	UUID EnTTSubsystem::AddEntity(bool shouldBeSerialized)
	{
		const auto entity = mRegistry->create();
		const auto id = GenerateId();

		mIDToEntity.emplace(id, entity);
		mEntityToID.emplace(entity, id);

		if (shouldBeSerialized)
			mShouldBeSerialized.emplace(id);

		return id;
	}

	entt::entity EnTTSubsystem::AddEntity(UUID id, bool shouldBeSerialized)
	{
		if (IsEntityValid(id))
			return mIDToEntity.at(id);

		const auto entity = mRegistry->create();

		mIDToEntity.emplace(id, entity);
		mEntityToID.emplace(entity, id);

		if (shouldBeSerialized)
			mShouldBeSerialized.emplace(id);

		return entity;
	}

	void EnTTSubsystem::RemoveEntity(UUID id)
	{
		mRegistry->destroy(mIDToEntity[id]);

		mIDToEntity.erase(id);
	}

	bool EnTTSubsystem::IsEntityValid(const UUID id)
	{
		return mIDToEntity.find(id) != mIDToEntity.end();
	}

	entt::entity EnTTSubsystem::GetEntity(UUID id) const
	{
		assert(mIDToEntity.find(id) != mIDToEntity.end() && "EnTTSubsystem::GetEntity() - No entity with that id exists");

		const entt::entity& entity = mIDToEntity.at(id);

		return entity;
	}

	UUID EnTTSubsystem::GetID(entt::entity entity) const
	{
		if (mEntityToID.count(entity) != 0)
			return mEntityToID.at(entity);

		return gInvalidID;
	}

	bool EnTTSubsystem::ShouldEntityBeSerialized(const UUID& id) const
	{
		if (mShouldBeSerialized.find(id) != mShouldBeSerialized.end())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	std::shared_ptr<entt::registry> EnTTSubsystem::GetRegistry()
	{
		return mRegistry;
	}
}
