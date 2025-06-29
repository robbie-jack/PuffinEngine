#include "rendering/camera_subsystem.h"

#include "component/transform_component_2d.h"
#include "component/rendering/2d/camera_component_2d.h"
#include "component/transform_component_3d.h"
#include "component/rendering/3d/camera_component_3d.h"
#include "core/settings_manager.h"
#include "ecs/entt_subsystem.h"
#include "scene/scene_graph_subsystem.h"
#include "core/signal_subsystem.h"

namespace puffin::rendering
{
	CameraSubsystem::CameraSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
        mName = "CameraSubsystem";
	}

	void CameraSubsystem::Initialize()
	{
		const auto enttSubsystem = subsystemManager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();
		const auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();
		const auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();

        const auto registry = enttSubsystem->GetRegistry();

        registry->on_construct<CameraComponent3D>().connect<&CameraSubsystem::OnUpdateCamera>(this);
        registry->on_update<CameraComponent3D>().connect<&CameraSubsystem::OnUpdateCamera>(this);
        registry->on_destroy<CameraComponent3D>().connect<&CameraSubsystem::OnDestroyCamera>(this);

		InitSettingsAndSignals();
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

    void CameraSubsystem::SetActiveCameraID(const UUID& id)
    {
		mActiveCameraID = id;
    }

    UUID CameraSubsystem::GetActiveCameraID() const
	{
		return mActiveCameraID;
	}

    bool CameraSubsystem::IsActiveCameraValid() const
    {
		return mActiveCameraID == gInvalidID;
    }

    void CameraSubsystem::InitSettingsAndSignals()
    {
		const auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		const auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();


    }

	void CameraSubsystem::UpdateActiveCamera()
	{
        if (mActivePlayCamID != gInvalidID)
        {
            mActiveCameraID = mActivePlayCamID;
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

	void CameraSubsystem::UpdateCameraComponent2D(const TransformComponent2D& transform, CameraComponent2D& camera)
	{
		// PFN_TODO_RENDERING - Implement
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
