#pragma once

#include <memory>
#include <thread>

#include "subsystem/engine_subsystem.h"

#include "TaskScheduler.h"

namespace puffin
{
	namespace core
	{
		class EnkiTSSubsystem : public EngineSubsystem
		{
		public:

			explicit EnkiTSSubsystem(const std::shared_ptr<Engine>& engine);
			~EnkiTSSubsystem() override = default;

			void PreInitialize(core::SubsystemManager* subsystemManager) override;
			void Initialize() override;
			void Deinitialize() override;

			std::string_view GetName() const override;

			std::shared_ptr<enki::TaskScheduler> GetTaskScheduler();
			uint32_t GetThreadCount() const;

		private:

			std::shared_ptr<enki::TaskScheduler> mTaskScheduler;

			uint32_t mThreadCount = 0;

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<core::EnkiTSSubsystem>()
		{
			return "EnkiTSSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<core::EnkiTSSubsystem>()
		{
			return entt::hs(GetTypeString<core::EnkiTSSubsystem>().data());
		}

		template<>
		inline void RegisterType<core::EnkiTSSubsystem>()
		{
			auto meta = entt::meta<core::EnkiTSSubsystem>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}