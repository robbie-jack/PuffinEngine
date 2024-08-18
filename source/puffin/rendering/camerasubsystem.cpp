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
        mActivePlayCamID = gInvalidID;
		mActiveCameraID = gInvalidID;
        mEditorCamID = gInvalidID;

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

    void CameraSubystem::OnDestroyCamera(entt::registry &registry, entt::entity entity)
    {
	    const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
        const auto id = enttSubsystem->GetID(entity);

        if (mActivePlayCamID == id)
        {
            mActivePlayCamID = gInvalidID;
        }

        mCachedCamActiveState.erase(id);
    }

    void CameraSubystem::OnUpdateEditorCameraFov(const float &editorCameraFov)
    {
	    const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
	    const auto registry = enttSubsystem->GetRegistry();

	    const auto entity = enttSubsystem->GetEntity(mEditorCamID);
        auto& camera = registry->get<CameraComponent3D>(entity);

        camera.prevFovY = camera.fovY;
        camera.fovY = editorCameraFov;
    }

	void CameraSubystem::InitEditorCamera()
	{
		// Crate editor cam 
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		mEditorCamID = GenerateId();
		mActiveCameraID = mEditorCamID;

		auto entity = enttSubsystem->AddEntity(mEditorCamID, false);

		auto& transform = registry->emplace<TransformComponent3D>(entity);
		transform.position = { 0.0f, 0.0f, 10.0f };

		auto& camera = registry->emplace<CameraComponent3D>(entity);

		mEditorCamSpeed = 25.0f;
	}

	void CameraSubystem::UpdateActiveCamera()
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

    void CameraSubystem::UpdateActivePlayCamera()
    {
        const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
        const auto registry = enttSubsystem->GetRegistry();
        const auto cameraView = registry->view<const TransformComponent3D, const CameraComponent3D>();

        for (auto [entity, transform, camera] : cameraView.each())
        {
            if (camera.active)
            {
                mActivePlayCamID = enttSubsystem->GetID(entity);

                break;
            }
        }
    }

	void CameraSubystem::UpdateCameras(double deltaTime)
	{
        if (mActivePlayCamID == gInvalidID)
        {
            UpdateActivePlayCamera();
        }

        if (mEngine->GetPlayState() == core::PlayState::Playing && mActiveCameraID != mActivePlayCamID)
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
        if (mEditorCamID != gInvalidID && mEditorCamID == mActiveCameraID)
        {
            const auto inputSubsystem = mEngine->GetSubsystem<input::InputSubsystem>();
            const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
            auto registry = enttSubsystem->GetRegistry();

            auto entity = enttSubsystem->GetEntity(mEditorCamID);
            auto &transform = registry->get<TransformComponent3D>(entity);
            auto &camera = registry->get<CameraComponent3D>(entity);

            if (inputSubsystem->GetCursorLocked() && mActiveCameraID == mEditorCamID) {
                // Camera Movement
                if (inputSubsystem->Pressed("EditorCamMoveRight") && !inputSubsystem->Pressed("EditorCamMoveLeft")) {
                    transform.position += camera.right * mEditorCamSpeed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveLeft") && !inputSubsystem->Pressed("EditorCamMoveRight")) {
                    transform.position -= camera.right * mEditorCamSpeed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveForward") &&
                    !inputSubsystem->Pressed("EditorCamMoveBackward")) {
                    transform.position += camera.direction * mEditorCamSpeed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveBackward") &&
                    !inputSubsystem->Pressed("EditorCamMoveForward")) {
                    transform.position -= camera.direction * mEditorCamSpeed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveUp") && !inputSubsystem->Pressed("EditorCamMoveDown")) {
                    transform.position += camera.up * mEditorCamSpeed * deltaTime;
                }

                if (inputSubsystem->Pressed("EditorCamMoveDown") && !inputSubsystem->Pressed("EditorCamMoveUp")) {
                    transform.position -= camera.up * mEditorCamSpeed * deltaTime;
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
		const auto renderSystem = mEngine->GetSubsystem<rendering::RenderSubsystemVK>();

		// Calculate direction & right vectors
		camera.direction = static_cast<glm::quat>(transform.orientationQuat) * glm::vec3(0.0f, 0.0f, -1.0f);
		camera.right = static_cast<glm::quat>(transform.orientationQuat) * glm::vec3(1.0f, 0.0f, 0.0f);

		camera.aspect = static_cast<float>(renderSystem->GetRenderExtent().width) / 
			static_cast<float>(renderSystem->GetRenderExtent().height);

		// Update view & projection matrices from updated direction and right vectors
		camera.view = glm::lookAt(static_cast<glm::vec3>(transform.position),
			static_cast<glm::vec3>(transform.position + camera.direction), static_cast<glm::vec3>(camera.up));

		camera.proj = glm::perspective(maths::DegToRad(camera.fovY), camera.aspect, camera.zNear, camera.zFar);
		camera.proj[1][1] *= -1; // Flips y-axis to match vulkan's coordinates system

		camera.viewProj = camera.proj * camera.view;
	}
}
