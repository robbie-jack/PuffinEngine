#pragma once

#include <unordered_map>
#include <entt/entity/registry.hpp>

#include "puffin/components/transform_component_3d.h"
#include "puffin/components/rendering/camera_component.h"
#include "puffin/core/system.h"
#include "puffin/types/uuid.h"

namespace puffin::rendering
{
	class CameraSubystem : public core::System
	{
	public:

		explicit CameraSubystem(const std::shared_ptr<core::Engine>& engine);

		~CameraSubystem() override = default;

		void startup();
		void begin_play();
		void update_subsystem();
		void end_play();

		void on_update_camera(entt::registry& registry, entt::entity entity);
        void on_destroy_camera(entt::registry& registry, entt::entity entity);

        void on_update_editor_camera_fov(const float& editor_camera_fov);

		PuffinID active_cam_id() const { return m_active_cam_id; }

	private:

        PuffinID m_active_cam_id = gInvalidID;
        PuffinID m_active_play_cam_id = gInvalidID;
		std::unordered_map<PuffinID, bool> m_cached_cam_active_state;
		PuffinID m_editor_cam_id = gInvalidID;
		float m_editor_cam_speed = 10.0f;

		void init_editor_camera();

        void update_active_play_camera();

		void update_cameras();
		void update_editor_camera();
		void update_camera_component(const TransformComponent3D& transform, CameraComponent3D& camera);

	};
}
