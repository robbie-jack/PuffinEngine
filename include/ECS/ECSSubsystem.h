#pragma once

#include "Core/System.h"
#include "Types/UUID.h"

#include <cassert>
#include <unordered_map>

namespace puffin::ecs
{
	class ECSSubsystem;

	class IComponentFactory
	{
	public:

		IComponentFactory(const std::shared_ptr<ECSSubsystem>& subsystem) : mSubsystem(subsystem) {}
			
		virtual ~IComponentFactory() = default;

		virtual void create(PuffinID id) = 0;

	protected:
		
		std::shared_ptr<ECSSubsystem> mSubsystem;

	};

	/*template<typename CompT>
	class ComponentFactory : public IComponentFactory
	{
	public:

		ComponentFactory(const std::shared_ptr<entt::registry>& registry) : IComponentFactory(registry) {}

		void create(PuffinID id) override;

	};*/

	class ComponentFactoryManager
	{
	public:

		ComponentFactoryManager(const std::shared_ptr<ECSSubsystem>& subsystem) : mSubsystem(subsystem) {}

		template<typename CompT>
		void registerComponent(const std::string& name)
		{
			const char* typeName = typeid(CompT).name();

			assert(mNameToComponentFactory.find(typeName) == mNameToComponentFactory.end() && "Registering system more than once");

			auto componentFactory = std::make_shared<ComponentFactory<CompT>>(mSubsystem);
		}

		template<typename CompT>
		void addComponent(PuffinID id)
		{
			
		}

	private:

		std::shared_ptr<ECSSubsystem> mSubsystem;

		std::unordered_map<std::string, std::shared_ptr<IComponentFactory>> mNameToComponentFactory;
		std::unordered_map<PuffinID, std::shared_ptr<IComponentFactory>> mIDToComponentFactory;

	};

	class ECSSubsystemProvider : public core::System
	{
	public:

		ECSSubsystemProvider(const std::shared_ptr<core::Engine>& engine) : System(engine) {}

		~ECSSubsystemProvider() override { mEngine = nullptr; }

	protected:

		friend class ECSSubsystem;

		virtual PuffinID createEntity(const std::string& name) = 0;
		virtual void addEntity(PuffinID id) = 0;
		virtual void destroyEntity(PuffinID id) = 0;
		virtual bool validEntity(PuffinID id) = 0;

		//template<typename CompT>
		//virtual void addComponent(PuffinID id);
	};

	class ECSSubsystem : public core::System, public std::enable_shared_from_this<ECSSubsystem>
	{
	public:

		ECSSubsystem(const std::shared_ptr<core::Engine>& engine);

		PuffinID createEntity(const std::string& name) const;
		void addEntity(PuffinID id);
		void destroyEntity(PuffinID id);
		bool validEntity(PuffinID id);

		template<typename CompT>
		void addComponent(PuffinID id);

	private:

		std::shared_ptr<ECSSubsystemProvider> mECSSubsystemProvider = nullptr;
		std::shared_ptr<ComponentFactoryManager> mComponentFactoryManager = nullptr;
	};
}