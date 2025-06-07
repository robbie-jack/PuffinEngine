#pragma once

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
	}

	namespace editor
	{
		class EditorCameraSubsystem : public core::Subsystem
		{
		public:

			explicit EditorCameraSubsystem(const std::shared_ptr<core::Engine>& engine);
			~EditorCameraSubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;
			void PostSceneLoad() override;

			void EndPlay() override;

			core::SubsystemType GetType() const override;

			void Update(double deltaTime) override;
			bool ShouldUpdate() override;

		private:

			void OnUpdateEditorCameraFov(const float& editorCameraFov);

			void InitSettingsAndSignals();

			void InitEditorCamera2D();
			void InitEditorCamera3D();

			void UpdateEditorCamera2D(double deltaTime);
			void UpdateEditorCamera3D(double deltaTime);

			UUID mEditorCamID = gInvalidID;
			float mEditorCamSpeed = 10.0f;
			Vector3f mEditorCamStartPosition = Vector3f(0.0f);

		};
	}
}
