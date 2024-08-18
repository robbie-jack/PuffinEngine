#pragma once

#include <unordered_map>
#include <entt/entity/registry.hpp>

#include "puffin/core/subsystem.h"
#include "puffin/types/uuid.h"

namespace puffin
{
	struct TransformComponent3D;
}

namespace puffin::rendering
{
	struct CameraComponent3D;

	class CameraSubystem : public core::Subsystem
	{
	public:

		explicit CameraSubystem(const std::shared_ptr<core::Engine>& engine);
		~CameraSubystem() override = default;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void BeginPlay() override;
		void EndPlay() override;

		void Update(double deltaTime) override;
		bool ShouldUpdate() override;

		void OnUpdateCamera(entt::registry& registry, entt::entity entity);
        void OnDestroyCamera(entt::registry& registry, entt::entity entity);

        void OnUpdateEditorCameraFov(const float& editorCameraFov);

		[[nodiscard]] UUID GetActiveCameraID() const { return mActiveCameraID; }

	private:

        UUID mActiveCameraID = gInvalidID;
        UUID mActivePlayCamID = gInvalidID;
		std::unordered_map<UUID, bool> mCachedCamActiveState;
		UUID mEditorCamID = gInvalidID;
		float mEditorCamSpeed = 10.0f;

		void InitEditorCamera();

		void UpdateActiveCamera();
        void UpdateActivePlayCamera();

		void UpdateCameras(double deltaTime);
		void UpdateEditorCamera(double deltaTime);
		void UpdateCameraComponent(const TransformComponent3D& transform, CameraComponent3D& camera);

	};
}
