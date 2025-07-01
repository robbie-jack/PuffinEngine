#pragma once

#include <unordered_map>
#include <memory>
#include <cassert>

#include "entt/meta/meta.hpp"

#include "subsystem/subsystem.h"

namespace puffin
{
	namespace core
	{
		class Engine;
		class EngineSubsystem;
		class EditorSubsystem;
		class GameplaySubsystem;
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
		~SubsystemManager();

		/*template<typename T>
		void RegisterSubsystem()
		{
			const char* typeName = typeid(T).name();

			assert(mSubsystemFactories.find(typeName) == mSubsystemFactories.end() && "SubsystemManager::RegisterSubsystem() - Attempting to register subsystem more than once");

			mSubsystemFactories.emplace(typeName, new SubsystemFactory<T>());

			CreateSubsystem(typeName);
		}*/

		void CreateAndPreInitializeEngineSubsystems();
		void CreateAndPreInitializeGameplaySubsystems();

		void DestroyEngineSubsystems();
		void DestroyGameplaySubsystems();

		template<typename T>
		T* CreateAndPreInitializeSubsystem()
		{
			auto type = entt::resolve<T>();
			auto typeId = type.id();

			if (m_subsystems.find(typeId) != m_subsystems.end())
				return dynamic_cast<T*>(m_subsystems.at(typeId));

			auto* subsystem = CreateSubsystem(typeId);
			subsystem->PreInitialize(this);

			return dynamic_cast<T*>(subsystem);
		}

		template<typename T>
		T* GetSubsystem()
		{
			auto type = entt::resolve<T>();
			auto typeId = type.id();

			if (m_subsystems.find(typeId) == m_subsystems.end())
				return nullptr;

			return static_cast<T*>(m_subsystems.at(typeId));
		}

		void GetEngineSubsystems(std::vector<EngineSubsystem*>& subsystems);
		void GetGameplaySubsystems(std::vector<GameplaySubsystem*>& subsystems);

		[[nodiscard]] window::WindowSubsystem* GetWindowSubsystem() const;
		[[nodiscard]] input::InputSubsystem* GetInputSubsystem() const;
		[[nodiscard]] rendering::RenderSubsystem* GetRenderSubsystem() const;

	private:

		Subsystem* CreateSubsystem(entt::id_type typeId);
		void DestroySubsystem(entt::id_type typeId);

		std::shared_ptr<Engine> m_engine = nullptr;

		//std::unordered_map<const char*, std::unique_ptr<ISubsystemFactory>> mSubsystemFactories;

		std::unordered_map<entt::id_type, Subsystem*> m_subsystems;

		std::vector<entt::id_type> m_engineSubsystem;
		std::vector<entt::id_type> m_editorSubsystem;
		std::vector<entt::id_type> m_gameplaySubsystems;

		entt::id_type m_windowSubsystem = 0;
		entt::id_type m_inputSubsystem = 0;
		entt::id_type m_renderSubsystem = 0;
	};
}
