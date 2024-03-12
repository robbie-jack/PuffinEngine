#include "ECS/ECSSubsystem.h"

namespace puffin::ecs
{
	/*template <typename CompT>
	void ComponentFactory<CompT>::create(PuffinID id)
	{
		if (mSubsystem)
		{
			
		}
		else
		{
			assert(mSubsystem != nullptr && "ECSSubvsystem was nullptr");
		}
	}*/

	ECSSubsystem::ECSSubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
	{
		mComponentFactoryManager = std::make_shared<ComponentFactoryManager>(shared_from_this());
	}

	PuffinID ECSSubsystem::createEntity(const std::string& name) const
	{
		if (mECSSubsystemProvider)
		{
			return mECSSubsystemProvider->createEntity(name);
		}

		return gInvalidID;
	}

	void ECSSubsystem::addEntity(PuffinID id)
	{
		if (mECSSubsystemProvider)
		{
			return mECSSubsystemProvider->addEntity(id);
		}
	}

	void ECSSubsystem::destroyEntity(PuffinID id)
	{
		if (mECSSubsystemProvider)
		{
			mECSSubsystemProvider->destroyEntity(id);
		}
	}

	bool ECSSubsystem::validEntity(PuffinID id)
	{
		if (mECSSubsystemProvider)
		{
			return mECSSubsystemProvider->validEntity(id);
		}

		return false;
	}

	template <typename CompT>
	void ECSSubsystem::addComponent(PuffinID id)
	{
		mECSSubsystemProvider->
	}
}
