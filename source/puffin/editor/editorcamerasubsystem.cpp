#include "puffin/editor/editorcamerasubsystem.h"

#include "puffin/ecs/enttsubsystem.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/rendering/2d/cameracomponent2d.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/input/inputsubsystem.h"

namespace puffin::editor
{
	EditorCameraSubsystem::EditorCameraSubsystem(const std::shared_ptr<core::Engine>& engine)
		: Subsystem(engine)
	{
		mName = "EditorCameraSubsystem";
	}

	core::SubsystemType EditorCameraSubsystem::GetType() const
	{
		return core::SubsystemType::Editor;
	}

	bool EditorCameraSubsystem::ShouldUpdate()
	{
		return true;
	}

	void EditorCameraSubsystem::OnUpdateEditorCameraFov(const float& editorCameraFov)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		const auto entity = enttSubsystem->GetEntity(mEditorCamID);
		auto& camera = registry->get<rendering::CameraComponent3D>(entity);

		camera.prevFovY = camera.fovY;
		camera.fovY = editorCameraFov;
	}

	void EditorCameraSubsystem::InitSettingsAndSignals()
	{
		const auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

		auto editorCameraFovSignal = signalSubsystem->GetOrCreateSignal("editor_camera_fov");
		editorCameraFovSignal->Connect(std::function([&]()
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				OnUpdateEditorCameraFov(settingsManager->Get<float>("editor", "camera_fov").value_or(60.0));
			}));

		mEditorCamSpeed = settingsManager->Get<float>("editor", "camera_speed").value_or(25.0);

		auto editorCameraSpeedSignal = signalSubsystem->GetOrCreateSignal("editor_camera_speed");
		editorCameraSpeedSignal->Connect(std::function([&]()
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				mEditorCamSpeed = settingsManager->Get<float>("editor", "camera_speed").value_or(25.0);
			}));

		mEditorCamStartPosition.x = settingsManager->Get<float>("editor", "camera_start_position_x").value_or(0.0);
		mEditorCamStartPosition.y = settingsManager->Get<float>("editor", "camera_start_position_y").value_or(0.0);
		mEditorCamStartPosition.z = settingsManager->Get<float>("editor", "camera_start_position_z").value_or(25.0);

		auto editorCamStartPositionXSignal = signalSubsystem->GetOrCreateSignal("editor_cam_start_position_x");
		editorCamStartPositionXSignal->Connect(std::function([&]()
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				mEditorCamStartPosition.x = settingsManager->Get<float>("editor", "camera_start_position_x").value_or(0.0);
			}));

		auto editorCamStartPositionYSignal = signalSubsystem->GetOrCreateSignal("editor_cam_start_position_y");
		editorCamStartPositionYSignal->Connect(std::function([&]()
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				mEditorCamStartPosition.y = settingsManager->Get<float>("editor", "camera_start_position_y").value_or(0.0);
			}));

		auto editorCamStartPositionZSignal = signalSubsystem->GetOrCreateSignal("editor_cam_start_position_z");
		editorCamStartPositionYSignal->Connect(std::function([&]()
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				mEditorCamStartPosition.z = settingsManager->Get<float>("editor", "camera_start_position_z").value_or(25.0);
			}));
	}

	void EditorCameraSubsystem::InitEditorCamera2D()
	{
		// PFN_TODO_RENDERING - Implement
	}

	void EditorCameraSubsystem::InitEditorCamera3D()
	{
		// Create editor cam
		auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		auto cameraSubsystem = mEngine->GetSubsystem<rendering::CameraSubsystem>();

		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		mEditorCamID = GenerateId();
		cameraSubsystem->SetActiveCameraID(mEditorCamID);

		auto entity = enttSubsystem->AddEntity(mEditorCamID, false);

		auto& transform = registry->emplace<TransformComponent3D>(entity);
		transform.position = mEditorCamStartPosition;

		auto& camera = registry->emplace<rendering::CameraComponent3D>(entity);

		camera.prevFovY = camera.fovY;
		camera.fovY = settingsManager->Get<float>("editor", "camera_fov").value_or(60.0);

		mEditorCamSpeed = 25.0f;
	}

	void EditorCameraSubsystem::UpdateEditorCamera2D(double deltaTime)
	{
		// PFN_TODO_RENDERING - Implement
	}

	void EditorCameraSubsystem::UpdateEditorCamera3D(double deltaTime)
	{
		auto cameraSubsystem = mEngine->GetSubsystem<rendering::CameraSubsystem>();

		if (mEditorCamID != gInvalidID && mEditorCamID == cameraSubsystem->GetActiveCameraID())
		{
			const auto inputSubsystem = mEngine->GetInputSubsystem();
			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			auto registry = enttSubsystem->GetRegistry();

			auto entity = enttSubsystem->GetEntity(mEditorCamID);
			auto& transform = registry->get<TransformComponent3D>(entity);
			auto& camera = registry->get<rendering::CameraComponent3D>(entity);

			{
				// Camera Movement
				if (inputSubsystem->IsActionDown("editor_cam_move_right")
					&& !inputSubsystem->IsActionDown("editor_cam_move_left")) {
					transform.position += camera.right * mEditorCamSpeed * deltaTime;
				}

				if (inputSubsystem->IsActionDown("editor_cam_move_left")
					&& !inputSubsystem->IsActionDown("editor_cam_move_right")) {
					transform.position -= camera.right * mEditorCamSpeed * deltaTime;
				}

				if (inputSubsystem->IsActionDown("editor_cam_move_forward")
					&& !inputSubsystem->IsActionDown("editor_cam_move_backward")) {
					transform.position += camera.direction * mEditorCamSpeed * deltaTime;
				}

				if (inputSubsystem->IsActionDown("editor_cam_move_backward")
					&& !inputSubsystem->IsActionDown("editor_cam_move_forward")) {
					transform.position -= camera.direction * mEditorCamSpeed * deltaTime;
				}

				if (inputSubsystem->IsActionDown("editor_cam_move_up")
					&& !inputSubsystem->IsActionDown("editor_cam_move_down")) {
					transform.position += camera.up * mEditorCamSpeed * deltaTime;
				}

				if (inputSubsystem->IsActionDown("editor_cam_move_down")
					&& !inputSubsystem->IsActionDown("editor_cam_move_up")) {
					transform.position -= camera.up * mEditorCamSpeed * deltaTime;
				}

				if (inputSubsystem->IsActionDown("editor_cam_look_around"))
				{
					// Mouse Rotation
					transform.orientationEulerAngles.yaw += inputSubsystem->GetMouseDeltaX();
					transform.orientationEulerAngles.pitch += inputSubsystem->GetMouseDeltaY();

					if (transform.orientationEulerAngles.pitch > 89.0f)
						transform.orientationEulerAngles.pitch = 89.0f;

					if (transform.orientationEulerAngles.pitch < -89.0f)
						transform.orientationEulerAngles.pitch = -89.0f;

					UpdateTransformOrientation(transform, transform.orientationEulerAngles);
				}
			}
		}
	}
}
