#include "puffin/rendering/camera_subsystem.h"

#include "puffin/components/transform_component_3d.h"
#include "puffin/components/rendering/camera_component.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/input/input_subsystem.h"
#include "puffin/rendering/vulkan/render_system_vk.h"
#include "puffin/scene/scene_graph.h"
#include "puffin/core/signal_subsystem.h"

namespace puffin::rendering
{
	CameraSubystem::CameraSubystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		m_engine->register_callback(core::ExecutionStage::StartupSubsystem, [&]() { startup(); }, "CameraSystem: startup");
		m_engine->register_callback(core::ExecutionStage::BeginPlay, [&]() { begin_play(); }, "CameraSystem: begin_play");
		m_engine->register_callback(core::ExecutionStage::UpdateSubsystem, [&]() { update_subsystem(); }, "CameraSystem: update");
		m_engine->register_callback(core::ExecutionStage::EndPlay, [&]() { end_play(); }, "CameraSystem: end_play", 210);

		const auto registry = m_engine->get_system<ecs::EnTTSubsystem>()->registry();

        registry->on_construct<CameraComponent3D>().connect<&CameraSubystem::on_update_camera>(this);
        registry->on_update<CameraComponent3D>().connect<&CameraSubystem::on_update_camera>(this);
        registry->on_destroy<CameraComponent3D>().connect<&CameraSubystem::on_destroy_camera>(this);

        auto signal_subsystem = m_engine->get_system<core::SignalSubsystem>();
        auto editor_camera_fov_signal = signal_subsystem->get_signal<float>("editor_camera_fov");
        if (!editor_camera_fov_signal)
        {
            editor_camera_fov_signal = signal_subsystem->create_signal<float>("editor_camera_fov");
        }

        editor_camera_fov_signal->connect(std::function([&](const float& editor_cam_fov)
        {
            on_update_editor_camera_fov(editor_cam_fov);
        }));
	}

	void CameraSubystem::startup()
	{
		init_editor_camera();
	}

	void CameraSubystem::begin_play()
	{
        update_active_play_camera();

        m_active_cam_id = m_active_play_cam_id;
	}

	void CameraSubystem::update_subsystem()
	{
		update_cameras();
	}

	void CameraSubystem::end_play()
	{
        m_active_play_cam_id = gInvalidID;
		m_active_cam_id = gInvalidID;
        m_editor_cam_id = gInvalidID;

        init_editor_camera();
	}

	void CameraSubystem::on_update_camera(entt::registry& registry, entt::entity entity)
	{
		const auto id = m_engine->get_system<ecs::EnTTSubsystem>()->get_id(entity);
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

    void CameraSubystem::on_destroy_camera(entt::registry &registry, entt::entity entity)
    {
        const auto id = m_engine->get_system<ecs::EnTTSubsystem>()->get_id(entity);

        if (m_active_play_cam_id == id)
        {
            m_active_play_cam_id = gInvalidID;
        }

        m_cached_cam_active_state.erase(id);
    }

    void CameraSubystem::on_update_editor_camera_fov(const float &editor_camera_fov)
    {
        auto entt_subsystem = m_engine->get_system<ecs::EnTTSubsystem>();
        auto registry = entt_subsystem->registry();

        auto entity = entt_subsystem->get_entity(m_editor_cam_id);
        auto& camera = registry->get<CameraComponent3D>(entity);

        camera.prev_fov_y = camera.fov_y;
        camera.fov_y = editor_camera_fov;
    }

	void CameraSubystem::init_editor_camera()
	{
		// Crate editor cam 
		auto entt_subsystem = m_engine->get_system<ecs::EnTTSubsystem>();
		auto registry = entt_subsystem->registry();

		m_editor_cam_id = generate_id();
		m_active_cam_id = m_editor_cam_id;

		auto entity = entt_subsystem->add_entity(m_editor_cam_id, false);

		auto& transform = registry->emplace<TransformComponent3D>(entity);
		transform.position = { 0.0f, 0.0f, 10.0f };

		auto& camera = registry->emplace<CameraComponent3D>(entity);

		m_editor_cam_speed = 25.0f;
	}

	void CameraSubystem::update_cameras()
	{
        if (m_active_play_cam_id == gInvalidID)
        {
            update_active_play_camera();
        }

        if (m_engine->play_state() == core::PlayState::Playing && m_active_cam_id != m_active_play_cam_id)
        {
            m_active_cam_id = m_active_play_cam_id;
        }

		update_editor_camera();

        const auto registry = m_engine->get_system<ecs::EnTTSubsystem>()->registry();
        const auto camera_view = registry->view<const TransformComponent3D, CameraComponent3D>();

		for (auto [entity, transform, camera] : camera_view.each())
		{
			update_camera_component(transform, camera);
		}
	}

	void CameraSubystem::update_editor_camera()
	{
        if (m_editor_cam_id != gInvalidID && m_editor_cam_id == m_active_cam_id)
        {
            const auto input_subsystem = m_engine->get_system<input::InputSubsystem>();
            auto entt_subsystem = m_engine->get_system<ecs::EnTTSubsystem>();
            auto registry = entt_subsystem->registry();

            auto entity = entt_subsystem->get_entity(m_editor_cam_id);
            auto &transform = registry->get<TransformComponent3D>(entity);
            auto &camera = registry->get<CameraComponent3D>(entity);

            if (input_subsystem->isCursorLocked() && m_active_cam_id == m_editor_cam_id) {
                // Camera Movement
                if (input_subsystem->pressed("EditorCamMoveRight") && !input_subsystem->pressed("EditorCamMoveLeft")) {
                    transform.position += camera.right * m_editor_cam_speed * m_engine->delta_time();
                }

                if (input_subsystem->pressed("EditorCamMoveLeft") && !input_subsystem->pressed("EditorCamMoveRight")) {
                    transform.position -= camera.right * m_editor_cam_speed * m_engine->delta_time();
                }

                if (input_subsystem->pressed("EditorCamMoveForward") &&
                    !input_subsystem->pressed("EditorCamMoveBackward")) {
                    transform.position += camera.direction * m_editor_cam_speed * m_engine->delta_time();
                }

                if (input_subsystem->pressed("EditorCamMoveBackward") &&
                    !input_subsystem->pressed("EditorCamMoveForward")) {
                    transform.position -= camera.direction * m_editor_cam_speed * m_engine->delta_time();
                }

                if (input_subsystem->pressed("EditorCamMoveUp") && !input_subsystem->pressed("EditorCamMoveDown")) {
                    transform.position += camera.up * m_editor_cam_speed * m_engine->delta_time();
                }

                if (input_subsystem->pressed("EditorCamMoveDown") && !input_subsystem->pressed("EditorCamMoveUp")) {
                    transform.position -= camera.up * m_editor_cam_speed * m_engine->delta_time();
                }

                // Mouse Rotation
                transform.orientation_euler_angles.yaw += input_subsystem->getMouseXOffset();
                transform.orientation_euler_angles.pitch += input_subsystem->getMouseYOffset();

                if (transform.orientation_euler_angles.pitch > 89.0f)
                    transform.orientation_euler_angles.pitch = 89.0f;

                if (transform.orientation_euler_angles.pitch < -89.0f)
                    transform.orientation_euler_angles.pitch = -89.0f;

                update_transform_orientation(transform, transform.orientation_euler_angles);
            }
        }
	}

	void CameraSubystem::update_camera_component(const TransformComponent3D& transform, CameraComponent3D& camera)
	{
		auto render_system = m_engine->get_system<rendering::RenderSystemVK>();

		// Calculate direction & right vectors
		camera.direction = static_cast<glm::quat>(transform.orientation_quat) * glm::vec3(0.0f, 0.0f, -1.0f);
		camera.right = static_cast<glm::quat>(transform.orientation_quat) * glm::vec3(1.0f, 0.0f, 0.0f);

		camera.aspect = static_cast<float>(render_system->render_extent().width) / 
			static_cast<float>(render_system->render_extent().height);

		// Update view & projection matrices from updated direction and right vectors
		camera.view = glm::lookAt(static_cast<glm::vec3>(transform.position),
			static_cast<glm::vec3>(transform.position + camera.direction), static_cast<glm::vec3>(camera.up));

		camera.proj = glm::perspective(maths::deg_to_rad(camera.fov_y), camera.aspect, camera.z_near, camera.z_far);
		camera.proj[1][1] *= -1; // Flips y-axis to match vulkan's coordinates system

		camera.view_proj = camera.proj * camera.view;
	}

    void CameraSubystem::update_active_play_camera()
    {
        const auto scene_graph = m_engine->get_system<scene::SceneGraph>();
        const auto entt_subsystem = m_engine->get_system<ecs::EnTTSubsystem>();
        const auto registry = entt_subsystem->registry();
        const auto camera_view = registry->view<const TransformComponent3D, const CameraComponent3D>();

        for (auto [entity, transform, camera] : camera_view.each())
        {
            if (camera.active)
            {
                m_active_play_cam_id = entt_subsystem->get_id(entity);

                break;
            }
        }
    }
}
