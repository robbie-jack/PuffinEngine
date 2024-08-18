#include "puffin/rendering/camerasubsystem.h"

#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/input/inputsubsystem.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/core/signalsubsystem.h"

namespace puffin::rendering
{
	CameraSubystem::CameraSubystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
        mName = "CameraSubsystem";
	}

	void CameraSubystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		const auto enttSubsystem = subsystemManager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();
		const auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();

        const auto registry = enttSubsystem->GetRegistry();

        registry->on_construct<CameraComponent3D>().connect<&CameraSubystem::OnUpdateCamera>(this);
        registry->on_update<CameraComponent3D>().connect<&CameraSubystem::OnUpdateCamera>(this);
        registry->on_destroy<CameraComponent3D>().connect<&CameraSubystem::OnDestroyCamera>(this);

        auto editorCameraFovSignal = signalSubsystem->GetSignal<float>("editorCameraFov");
        if (!editorCameraFovSignal)
        {
            editorCameraFovSignal = signalSubsystem->CreateSignal<float>("editorCameraFov");
        }

        editorCameraFovSignal->Connect(std::function([&](const float& editorCamFov)
        {
            OnUpdateEditorCameraFov(editorCamFov);
        }));

        InitEditorCamera();
	}

	void CameraSubystem::Deinitialize()
	{
	}

	void CameraSubystem::BeginPlay()
	{
        UpdateActivePlayCamera();
        UpdateActiveCamera();
	}

	void CameraSubystem::EndPlay()
	{
        m_active_play_cam_id = gInvalidID;
		mActiveCameraID = gInvalidID;
        m_editor_cam_id = gInvalidID;

        InitEditorCamera();
	}

	void CameraSubystem::Update(double deltaTime)
	{
        UpdateCameras(deltaTime);
	}

	bool CameraSubystem::ShouldUpdate()
	{
		return true;
	}

	void CameraSubystem::OnUpdateCamera(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = enttSubsystem->GetID(entity);
		const auto& camera = registry.get<CameraComponent3D>(entity);

        if (m_cached_cam_active_state.find(id) == m_cached_cam_active_state.end())
        {
            m_cached_cam_active_state.emplace(id, camera.active);
        }

		if (m_cached_cam_active_state.at(id) != camera.active)
		{
			if (camera.active)
			{
                m_active_play_cam_id = id;
			}
			else
			{
                m_active_play_cam_id = gInvalidID;
			}

            m_cached_cam_active_state.at(id) = camera.active;
		}
	}

    void CameraSubystem::OnDestroyCamera(entt::registry &registry, entt::entity entity)
    {
	    const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
        const auto id = enttSubsystem->GetID(entity);

        if (m_active_play_cam_id == id)
        {
            m_active_play_cam_id = gInvalidID;
        }

        m_cached_cam_active_state.erase(id);
    }

    void CameraSubystem::OnUpdateEditorCameraFov(const float &editorCameraFov)
    {
	    const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
	    const auto registry = enttSubsystem->GetRegistry();

	    const auto entity = enttSubsystem->GetEntity(m_editor_cam_id);
        auto& camera = registry->get<CameraComponent3D>(entity);

        camera.prevFovY = camera.fovY;
        camera.fovY = editorCameraFov;
    }

	void CameraSubystem::InitEditorCamera()
	{
		// Crate editor cam 
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		m_editor_cam_id = GenerateId();
		mActiveCameraID = m_editor_cam_id;

		auto entity = enttSubsystem->AddEntity(m_editor_cam_id, false);

		auto& transform = registry->emplace<TransformComponent3D>(entity);
		transform.position = { 0.0f, 0.0f, 10.0f };

		auto& camera = registry->emplace<CameraComponent3D>(entity);

		m_editor_cam_speed = 25.0f;
	}

	void CameraSubystem::UpdateActiveCamera()
	{
        if (m_active_play_cam_id != gInvalidID)
        {
            mActiveCameraID = m_active_play_cam_id;
        }
        else
        {
            mActiveCameraID = m_editor_cam_id;
        }
	}

    void CameraSubystem::UpdateActivePlayCamera()
    {
        const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
        const auto registry = enttSubsystem->GetRegistry();
        const auto cameraView = registry->view<const TransformComponent3D, const CameraComponent3D>();

        for (auto [entity, transform, camera] : cameraView.each())
        {
            if (camera.active)
            {
                m_active_play_cam_id = enttSubsystem->GetID(entity);

                break;
            }
        }
    }

	void CameraSubystem::UpdateCameras(double deltaTime)
	{
        if (m_active_play_cam_id == gInvalidID)
        {
            UpdateActivePlayCamera();
        }

        if (mEngine->GetPlayState() == core::PlayState::Playing && mActiveCameraID != m_active_play_cam_id)
        {
            UpdateActiveCamera();
        }

		UpdateEditorCamera(deltaTime);

        const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
        const auto registry = enttSubsystem->GetRegistry();
        const auto cameraView = registry->view<const TransformComponent3D, CameraComponent3D>();

		for (auto [entity, transform, camera] : cameraView.each())
		{
			UpdateCameraComponent(transform, camera);
		}
	}

	void CameraSubystem::UpdateEditorCamera(double deltaTime)
	{
        if (m_editor_cam_id != gInvalidID && m_editor_cam_id == mActiveCameraID)
        {
            const auto inputSubsystem = mEngine->GetSubsystem<input::InputSubsystem>();
            const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            auto registry = enttSubsystem->GetRegistry();

            auto entity = enttSubsystem->GetEntity(m_editor_cam_id);
            auto &transform = registry->get<TransformComponent3D>(entity);
            auto &camera = registry->get<CameraComponent3D>(entity);

            if (inputSubsystem->GetCursorLocked() && mActiveCameraID == m_editor_cam_id) {
                // Camera Movement
                if (inputSubsystem->Pressed("EditorCamMoveRight") && !inputSubsystem->Pressed("EditorCamMoveLeft")) {
                    transform.position += camera.right * m_editor_cam_speed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveLeft") && !inputSubsystem->Pressed("EditorCamMoveRight")) {
                    transform.position -= camera.right * m_editor_cam_speed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveForward") &&
                    !inputSubsystem->Pressed("EditorCamMoveBackward")) {
                    transform.position += camera.direction * m_editor_cam_speed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveBackward") &&
                    !inputSubsystem->Pressed("EditorCamMoveForward")) {
                    transform.position -= camera.direction * m_editor_cam_speed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveUp") && !inputSubsystem->Pressed("EditorCamMoveDown")) {
                    transform.position += camera.up * m_editor_cam_speed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveDown") && !inputSubsystem->Pressed("EditorCamMoveUp")) {
                    transform.position -= camera.up * m_editor_cam_speed * deltaTime;
                }

                // Mouse Rotation
                transform.orientationEulerAngles.yaw += inputSubsystem->GetMouseXOffset();
                transform.orientationEulerAngles.pitch += inputSubsystem->GetMouseYOffset();

                if (transform.orientationEulerAngles.pitch > 89.0f)
                    transform.orientationEulerAngles.pitch = 89.0f;

                if (transform.orientationEulerAngles.pitch < -89.0f)
                    transform.orientationEulerAngles.pitch = -89.0f;

                UpdateTransformOrientation(transform, transform.orientationEulerAngles);
            }
        }
	}

	void CameraSubystem::UpdateCameraComponent(const TransformComponent3D& transform, CameraComponent3D& camera)
	{
		const auto renderSystem = mEngine->GetSubsystem<rendering::RenderSubystemVK>();

		// Calculate direction & right vectors
		camera.direction = static_cast<glm::quat>(transform.orientationQuat) * glm::vec3(0.0f, 0.0f, -1.0f);
		camera.right = static_cast<glm::quat>(transform.orientationQuat) * glm::vec3(1.0f, 0.0f, 0.0f);

		camera.aspect = static_cast<float>(renderSystem->GetRenderExtent().width) / 
			static_cast<float>(renderSystem->GetRenderExtent().height);

		// Update view & projection matrices from updated direction and right vectors
		camera.view = glm::lookAt(static_cast<glm::vec3>(transform.position),
			static_cast<glm::vec3>(transform.position + camera.direction), static_cast<glm::vec3>(camera.up));

		camera.proj = glm::perspective(maths::deg_to_rad(camera.fovY), camera.aspect, camera.zNear, camera.zFar);
		camera.proj[1][1] *= -1; // Flips y-axis to match vulkan's coordinates system

		camera.viewProj = camera.proj * camera.view;
	}
}
