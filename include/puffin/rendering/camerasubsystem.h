#pragma once

#include <unordered_map>
#include <entt/entity/registry.hpp>

#include "puffin/core/subsystem.h"
#include "puffin/types/uuid.h"
#include "puffin/types/vector3.h"

namespace puffin
{
	struct TransformComponent2D;
	struct TransformComponent3D;

	namespace rendering
	{
		struct CameraComponent2D;
		struct CameraComponent3D;

		class CameraSubsystem : public core::Subsystem
		{
		public:

			explicit CameraSubsystem(const std::shared_ptr<core::Engine>& engine);
			~CameraSubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			void BeginPlay() override;
			void EndPlay() override;

			void Update(double deltaTime) override;
			bool ShouldUpdate() override;

			void OnUpdateCamera(entt::registry& registry, entt::entity entity);
			void OnDestroyCamera(entt::registry& registry, entt::entity entity);

			void SetActiveCameraID(const UUID& id);
			[[nodiscard]] UUID GetActiveCameraID() const;
			[[nodiscard]] UUID GetActivePlayCameraID() const;

		private:

			void InitSettingsAndSignals();

			void UpdateActiveCamera();
			void UpdateActivePlayCamera();

			void UpdateCameras(double deltaTime);

			void UpdateCameraComponent2D(const TransformComponent2D& transform, CameraComponent2D& camera);
			void UpdateCameraComponent3D(const TransformComponent3D& transform, CameraComponent3D& camera);

			UUID mActiveCameraID = gInvalidID;
			UUID mActivePlayCamID = gInvalidID;
			std::unordered_map<UUID, bool> mCachedCamActiveState;

		};
	}
}