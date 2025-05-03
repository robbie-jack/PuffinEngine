#include "puffin/rendering/camerasubsystem.h"

#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/rendering/2d/cameracomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/input/inputsubsystem.h"
//#include "puffin/rendering/vulkan/rendersubsystemvk.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/core/signalsubsystem.h"

namespace puffin::rendering
{
	CameraSubsystem::CameraSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
        mName = "CameraSubsystem";
	}

	void CameraSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		const auto enttSubsystem = subsystemManager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();
		const auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();
		const auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();

        const auto registry = enttSubsystem->GetRegistry();

        registry->on_construct<CameraComponent3D>().connect<&CameraSubsystem::OnUpdateCamera>(this);
        registry->on_update<CameraComponent3D>().connect<&CameraSubsystem::OnUpdateCamera>(this);
        registry->on_destroy<CameraComponent3D>().connect<&CameraSubsystem::OnDestroyCamera>(this);

		InitSettingsAndSignals();

        InitEditorCamera3D();
	}

	void CameraSubsystem::Deinitialize()
	{
	}

	void CameraSubsystem::BeginPlay()
	{
        UpdateActivePlayCamera();
        UpdateActiveCamera();
	}

	void CameraSubsystem::EndPlay()
	{
        mActivePlayCamID = gInvalidID;
		mActiveCameraID = gInvalidID;
        mEditorCamID = gInvalidID;

        InitEditorCamera3D();
	}

	void CameraSubsystem::Update(double deltaTime)
	{
        UpdateCameras(deltaTime);
	}

	bool CameraSubsystem::ShouldUpdate()
	{
		return true;
	}

	void CameraSubsystem::OnUpdateCamera(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = enttSubsystem->GetID(entity);
		const auto& camera = registry.get<CameraComponent3D>(entity);

        if (mCachedCamActiveState.find(id) == mCachedCamActiveState.end())
        {
            mCachedCamActiveState.emplace(id, camera.active);
        }

		if (mCachedCamActiveState.at(id) != camera.active)
		{
			if (camera.active)
			{
                mActivePlayCamID = id;
			}
			else
			{
                mActivePlayCamID = gInvalidID;
			}

            mCachedCamActiveState.at(id) = camera.active;
		}
	}

    void CameraSubsystem::OnDestroyCamera(entt::registry &registry, entt::entity entity)
    {
	    const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
        const auto id = enttSubsystem->GetID(entity);

        if (mActivePlayCamID == id)
        {
            mActivePlayCamID = gInvalidID;
        }

        mCachedCamActiveState.erase(id);
    }

    void CameraSubsystem::OnUpdateEditorCameraFov(const float &editorCameraFov)
    {
	    const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
	    const auto registry = enttSubsystem->GetRegistry();

	    const auto entity = enttSubsystem->GetEntity(mEditorCamID);
        auto& camera = registry->get<CameraComponent3D>(entity);

        camera.prevFovY = camera.fovY;
        camera.fovY = editorCameraFov;
    }

    void CameraSubsystem::InitSettingsAndSignals()
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

    void CameraSubsystem::InitEditorCamera2D()
    {
		// PFN_TODO - Implement
    }

    void CameraSubsystem::InitEditorCamera3D()
	{
		// Create editor cam
		auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		mEditorCamID = GenerateId();
		mActiveCameraID = mEditorCamID;

		auto entity = enttSubsystem->AddEntity(mEditorCamID, false);

		auto& transform = registry->emplace<TransformComponent3D>(entity);
		transform.position = mEditorCamStartPosition;

		auto& camera = registry->emplace<CameraComponent3D>(entity);

		camera.prevFovY = camera.fovY;
		camera.fovY = settingsManager->Get<float>("editor", "camera_fov").value_or(60.0);

		mEditorCamSpeed = 25.0f;
	}

	void CameraSubsystem::UpdateActiveCamera()
	{
        if (mActivePlayCamID != gInvalidID)
        {
            mActiveCameraID = mActivePlayCamID;
        }
        else
        {
            mActiveCameraID = mEditorCamID;
        }
	}

    void CameraSubsystem::UpdateActivePlayCamera()
    {
        const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
        const auto registry = enttSubsystem->GetRegistry();

		const auto camera2DView = registry->view<const TransformComponent2D, const CameraComponent2D>();
		for (auto [entity, transform, camera] : camera2DView.each())
		{
			if (camera.active)
			{
				mActivePlayCamID = enttSubsystem->GetID(entity);

				break;
			}
		}

        const auto camera3DView = registry->view<const TransformComponent3D, const CameraComponent3D>();
        for (auto [entity, transform, camera] : camera3DView.each())
        {
            if (camera.active)
            {
                mActivePlayCamID = enttSubsystem->GetID(entity);

                break;
            }
        }
    }

	void CameraSubsystem::UpdateCameras(double deltaTime)
	{
        if (mActivePlayCamID == gInvalidID)
        {
            UpdateActivePlayCamera();
        }

        if (mEngine->GetPlayState() == core::PlayState::Playing && mActiveCameraID != mActivePlayCamID)
        {
            UpdateActiveCamera();
        }

		UpdateEditorCamera2D(deltaTime);
		UpdateEditorCamera3D(deltaTime);

        const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
        const auto registry = enttSubsystem->GetRegistry();

		const auto camera2DView = registry->view<const TransformComponent2D, CameraComponent2D>();
		for (auto [entity, transform, camera] : camera2DView.each())
		{
			UpdateCameraComponent2D(transform, camera);
		}

		const auto camera3DView = registry->view<const TransformComponent3D, CameraComponent3D>();
		for (auto [entity, transform, camera] : camera3DView.each())
		{
			UpdateCameraComponent3D(transform, camera);
		}
	}

	void CameraSubsystem::UpdateEditorCamera2D(double deltaTime)
	{
		// PFN_TODO_RENDERING - Implement
	}

	void CameraSubsystem::UpdateCameraComponent2D(const TransformComponent2D& transform, CameraComponent2D& camera)
	{
		// PFN_TODO_RENDERING - Implement
	}

	void CameraSubsystem::UpdateEditorCamera3D(double deltaTime)
	{
        if (mEditorCamID != gInvalidID && mEditorCamID == mActiveCameraID)
        {
            const auto inputSubsystem = mEngine->GetInputSubsystem();
            const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            auto registry = enttSubsystem->GetRegistry();

            auto entity = enttSubsystem->GetEntity(mEditorCamID);
            auto &transform = registry->get<TransformComponent3D>(entity);
            auto &camera = registry->get<CameraComponent3D>(entity);

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

	void CameraSubsystem::UpdateCameraComponent3D(const TransformComponent3D& transform, CameraComponent3D& camera)
	{
		// PFN_TODO_RENDERING - Re-implement when 3d rendering code is being re-implemented

		//const auto renderSystem = mEngine->GetSubsystem<rendering::RenderSubsystemVK>();

		//// Calculate direction & right vectors
		//camera.direction = static_cast<glm::quat>(transform.orientationQuat) * glm::vec3(0.0f, 0.0f, -1.0f);
		//camera.right = static_cast<glm::quat>(transform.orientationQuat) * glm::vec3(1.0f, 0.0f, 0.0f);

		//camera.aspect = static_cast<float>(renderSystem->GetRenderExtent().width) / 
		//	static_cast<float>(renderSystem->GetRenderExtent().height);

		//// Update view & projection matrices from updated direction and right vectors
		//camera.view = glm::lookAt(static_cast<glm::vec3>(transform.position),
		//	static_cast<glm::vec3>(transform.position + camera.direction), static_cast<glm::vec3>(camera.up));

		//camera.proj = glm::perspective(maths::DegToRad(camera.fovY), camera.aspect, camera.zNear, camera.zFar);
		//camera.proj[1][1] *= -1; // Flips y-axis to match vulkan's coordinates system

		//camera.viewProj = camera.proj * camera.view;
	}
}
