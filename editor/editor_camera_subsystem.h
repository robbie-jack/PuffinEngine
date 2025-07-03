#pragma once

#include "subsystem/editor_subsystem.h"
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
	}

	namespace editor
	{
		class EditorCameraSubsystem : public core::EditorSubsystem
		{
		public:

			explicit EditorCameraSubsystem(const std::shared_ptr<core::Engine>& engine);
			~EditorCameraSubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;
			void PostSceneLoad() override;

			void EndPlay() override;

			void Update(double deltaTime) override;
			bool ShouldUpdate() override;

			std::string_view GetName() const override;

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

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<editor::EditorCameraSubsystem>()
		{
			return "EditorCameraSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<editor::EditorCameraSubsystem>()
		{
			return entt::hs(GetTypeString<editor::EditorCameraSubsystem>().data());
		}

		template<>
		inline void RegisterType<editor::EditorCameraSubsystem>()
		{
			auto meta = entt::meta<editor::EditorCameraSubsystem>()
				.base<core::EditorSubsystem>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
