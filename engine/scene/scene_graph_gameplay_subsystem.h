#pragma once

#include "core/subsystem.h"

namespace puffin::scene
{
	class SceneGraphGameplaySubsystem : public core::Subsystem
	{
	public:

		explicit SceneGraphGameplaySubsystem(const std::shared_ptr<core::Engine>& engine);
		~SceneGraphGameplaySubsystem() override = default;

		void Initialize(core::SubsystemManager* subsystemManager) override;

		void BeginPlay() override;
		void EndPlay() override;

		[[nodiscard]] core::SubsystemType GetType() const override;

		void Update(double deltaTime) override;
		bool ShouldUpdate() override;

		void FixedUpdate(double fixedTime) override;
		bool ShouldFixedUpdate() override;

	};
}
