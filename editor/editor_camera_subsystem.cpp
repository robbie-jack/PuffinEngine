#include "editor_camera_subsystem.h"

#include "ecs/entt_subsystem.h"
#include "component/transform_component_2d.h"
#include "component/transform_component_3d.h"
#include "component/rendering/2d/camera_component_2d.h"
#include "component/rendering/3d/camera_component_3d.h"
#include "core/settings_manager.h"
#include "rendering/camera_subsystem.h"
#include "input/input_subsystem.h"

namespace puffin::editor
{
	EditorCameraSubsystem::EditorCameraSubsystem(const std::shared_ptr<core::Engine>& engine)
		: EditorSubsystem(engine)
	{
	}

	void EditorCameraSubsystem::PreInitialize(core::SubsystemManager* subsystemManager)
	{
		EditorSubsystem::PreInitialize(subsystemManager);

		subsystemManager->CreateAndPreInitializeSubsystem<core::SignalSubsystem>();
		subsystemManager->CreateAndPreInitializeSubsystem<core::SettingsManager>();
	}

	void EditorCameraSubsystem::Initialize()
	{
		Subsystem::Initialize();

		InitSettingsAndSignals();
	}

	void EditorCameraSubsystem::Deinitialize()
	{
		Subsystem::Deinitialize();
	}

	void EditorCameraSubsystem::PostSceneLoad()
	{
		Subsystem::PostSceneLoad();

		auto currentSceneType = m_engine->GetCurrentSceneType();

		if (currentSceneType ==  scene::SceneType::Scene2D)
		{
			InitEditorCamera2D();
		}

		if (currentSceneType == scene::SceneType::Scene3D)
		{
			InitEditorCamera3D();
		}
	}

	void EditorCameraSubsystem::EndPlay()
	{
		Subsystem::EndPlay();

		mEditorCamID = gInvalidID;

		auto currentSceneType = m_engine->GetCurrentSceneType();

		if (currentSceneType == scene::SceneType::Scene2D)
		{
			InitEditorCamera2D();
		}

		if (currentSceneType == scene::SceneType::Scene3D)
		{
			InitEditorCamera3D();
		}
	}

	void EditorCameraSubsystem::Update(double deltaTime)
	{
		EditorSubsystem::Update(deltaTime);

		auto currentSceneType = m_engine->GetCurrentSceneType();

		if (currentSceneType == scene::SceneType::Scene2D)
		{
			UpdateEditorCamera2D(deltaTime);
		}

		if (currentSceneType == scene::SceneType::Scene3D)
		{
			UpdateEditorCamera3D(deltaTime);
		}
	}

	bool EditorCameraSubsystem::ShouldUpdate()
	{
		return true;
	}

	std::string_view EditorCameraSubsystem::GetName() const
	{
		return reflection::GetTypeString<EditorCameraSubsystem>();
	}

	void EditorCameraSubsystem::OnUpdateEditorCameraFov(const float& editorCameraFov)
	{
		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		const auto entity = enttSubsystem->GetEntity(mEditorCamID);
		auto& camera = registry->get<rendering::CameraComponent3D>(entity);

		camera.prevFovY = camera.fovY;
		camera.fovY = editorCameraFov;
	}

	void EditorCameraSubsystem::InitSettingsAndSignals()
	{
		const auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();
		const auto signalSubsystem = m_engine->GetSubsystem<core::SignalSubsystem>();

		auto editorCameraFovSignal = signalSubsystem->GetOrCreateSignal("editor_camera_fov");
		editorCameraFovSignal->Connect(std::function([&]()
		{
			auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

			OnUpdateEditorCameraFov(settingsManager->Get<float>("editor", "camera_fov").value_or(60.0));
		}));

		mEditorCamSpeed = settingsManager->Get<float>("editor", "camera_speed").value_or(25.0);

		auto editorCameraSpeedSignal = signalSubsystem->GetOrCreateSignal("editor_camera_speed");
		editorCameraSpeedSignal->Connect(std::function([&]()
		{
			auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

			mEditorCamSpeed = settingsManager->Get<float>("editor", "camera_speed").value_or(25.0);
		}));

		mEditorCamStartPosition.x = settingsManager->Get<float>("editor", "camera_start_position_x").value_or(0.0);
		mEditorCamStartPosition.y = settingsManager->Get<float>("editor", "camera_start_position_y").value_or(0.0);
		mEditorCamStartPosition.z = settingsManager->Get<float>("editor", "camera_start_position_z").value_or(25.0);

		auto editorCamStartPositionXSignal = signalSubsystem->GetOrCreateSignal("editor_cam_start_position_x");
		editorCamStartPositionXSignal->Connect(std::function([&]()
		{
			auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

			mEditorCamStartPosition.x = settingsManager->Get<float>("editor", "camera_start_position_x").value_or(0.0);
		}));

		auto editorCamStartPositionYSignal = signalSubsystem->GetOrCreateSignal("editor_cam_start_position_y");
		editorCamStartPositionYSignal->Connect(std::function([&]()
		{
			auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

			mEditorCamStartPosition.y = settingsManager->Get<float>("editor", "camera_start_position_y").value_or(0.0);
		}));

		auto editorCamStartPositionZSignal = signalSubsystem->GetOrCreateSignal("editor_cam_start_position_z");
		editorCamStartPositionYSignal->Connect(std::function([&]()
		{
			auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

			mEditorCamStartPosition.z = settingsManager->Get<float>("editor", "camera_start_position_z").value_or(25.0);
		}));
	}

	void EditorCameraSubsystem::InitEditorCamera2D()
	{
		// Create editor cam
		auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();
		auto cameraSubsystem = m_engine->GetSubsystem<rendering::CameraSubsystem>();

		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		mEditorCamID = GenerateId();
		cameraSubsystem->SetActiveCameraID(mEditorCamID);

		auto entity = enttSubsystem->AddEntity(mEditorCamID, false);

		auto& transform = registry->emplace<TransformComponent2D>(entity);
		transform.position = { mEditorCamStartPosition.x, mEditorCamStartPosition.y };

		auto& camera = registry->emplace<rendering::CameraComponent2D>(entity);

		camera.zoom = settingsManager->Get<float>("editor", "camera_zoom").value_or(1.0);
	}

	void EditorCameraSubsystem::InitEditorCamera3D()
	{
		// Create editor cam
		auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();
		auto cameraSubsystem = m_engine->GetSubsystem<rendering::CameraSubsystem>();

		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
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
		auto cameraSubsystem = m_engine->GetSubsystem<rendering::CameraSubsystem>();

		if (mEditorCamID != gInvalidID && mEditorCamID == cameraSubsystem->GetActiveCameraID())
		{
			const auto inputSubsystem = m_engine->GetInputSubsystem();
			const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			auto registry = enttSubsystem->GetRegistry();

			auto entity = enttSubsystem->GetEntity(mEditorCamID);
			auto& transform = registry->get<TransformComponent2D>(entity);
			auto& camera = registry->get<rendering::CameraComponent2D>(entity);

			// Camera Movement
			if (inputSubsystem->IsActionDown("editor_cam_move_right")
				&& !inputSubsystem->IsActionDown("editor_cam_move_left")) {
				transform.position.x += mEditorCamSpeed * deltaTime;
			}

			if (inputSubsystem->IsActionDown("editor_cam_move_left")
				&& !inputSubsystem->IsActionDown("editor_cam_move_right")) {
				transform.position.x -= mEditorCamSpeed * deltaTime;
			}

			if (inputSubsystem->IsActionDown("editor_cam_move_forward")
				&& !inputSubsystem->IsActionDown("editor_cam_move_backward")) {
				transform.position.y -= mEditorCamSpeed * deltaTime;
			}

			if (inputSubsystem->IsActionDown("editor_cam_move_backward")
				&& !inputSubsystem->IsActionDown("editor_cam_move_forward")) {
				transform.position.y += mEditorCamSpeed * deltaTime;
			}
		}
	}

	void EditorCameraSubsystem::UpdateEditorCamera3D(double deltaTime)
	{
		auto cameraSubsystem = m_engine->GetSubsystem<rendering::CameraSubsystem>();

		if (mEditorCamID != gInvalidID && mEditorCamID == cameraSubsystem->GetActiveCameraID())
		{
			const auto inputSubsystem = m_engine->GetInputSubsystem();
			const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
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
