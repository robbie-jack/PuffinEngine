#pragma once

#include "subsystem/gameplay_subsystem.h"

namespace puffin
{
	namespace scene
	{
		class SceneGraphGameplaySubsystem : public core::GameplaySubsystem
		{
		public:

			explicit SceneGraphGameplaySubsystem(const std::shared_ptr<core::Engine>& engine);
			~SceneGraphGameplaySubsystem() override = default;

			void PreInitialize(core::SubsystemManager* subsystemManager) override;
			void Initialize() override;

			void BeginPlay() override;
			void EndPlay() override;

			void Update(double deltaTime) override;
			bool ShouldUpdate() override;

			void FixedUpdate(double fixedTime) override;
			bool ShouldFixedUpdate() override;

			std::string_view GetName() const override;

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<scene::SceneGraphGameplaySubsystem>()
		{
			return "SceneGraphGameplaySubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<scene::SceneGraphGameplaySubsystem>()
		{
			return entt::hs(GetTypeString<scene::SceneGraphGameplaySubsystem>().data());
		}

		template<>
		inline void RegisterType<scene::SceneGraphGameplaySubsystem>()
		{
			auto meta = entt::meta<scene::SceneGraphGameplaySubsystem>()
				.base<core::GameplaySubsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
