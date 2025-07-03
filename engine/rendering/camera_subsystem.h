#pragma once

#include <unordered_map>
#include <entt/entity/registry.hpp>

#include "subsystem/engine_subsystem.h"
#include "types/uuid.h"
#include "types/vector3.h"

namespace puffin
{
	struct TransformComponent2D;
	struct TransformComponent3D;

	namespace rendering
	{
		struct CameraComponent2D;
		struct CameraComponent3D;

		class CameraSubsystem : public core::EngineSubsystem
		{
		public:

			explicit CameraSubsystem(const std::shared_ptr<core::Engine>& engine);
			~CameraSubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			void BeginPlay() override;
			void EndPlay() override;

			void Update(double deltaTime) override;
			bool ShouldUpdate() override;

			std::string_view GetName() const override;

			void OnUpdateCamera(entt::registry& registry, entt::entity entity);
			void OnDestroyCamera(entt::registry& registry, entt::entity entity);

			void SetActiveCameraID(const UUID& id);
			[[nodiscard]] UUID GetActiveCameraID() const;
			[[nodiscard]] UUID GetActivePlayCameraID() const;
			bool IsActiveCameraValid() const;

		private:

			void InitSettingsAndSignals();

			void UpdateActiveCamera();
			void UpdateActivePlayCamera();

			void UpdateCameras(double deltaTime);

			void UpdateCameraComponent2D(const TransformComponent2D& transform, CameraComponent2D& camera);
			void UpdateCameraComponent3D(const TransformComponent3D& transform, CameraComponent3D& camera);

			UUID mActiveCameraID = gInvalidID;
			UUID mActivePlayCamID = gInvalidID;
			std::unordered_map<UUID, bool> mCachedCamActiveState;

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<rendering::CameraSubsystem>()
		{
			return "CameraSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<rendering::CameraSubsystem>()
		{
			return entt::hs(GetTypeString<rendering::CameraSubsystem>().data());
		}

		template<>
		inline void RegisterType<rendering::CameraSubsystem>()
		{
			auto meta = entt::meta<rendering::CameraSubsystem>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}