#pragma once

#include "input/input_subsystem.h"

namespace puffin
{
	namespace input
	{
		class RaylibInputSubsystem : public InputSubsystem
		{
		public:

			explicit RaylibInputSubsystem(const std::shared_ptr<core::Engine>& engine);
			~RaylibInputSubsystem() override = default;

			std::string_view GetName() const override;

			bool IsKeyPressed(KeyboardKey key) const override;
			bool IsKeyDown(KeyboardKey key) const override;
			bool IsKeyReleased(KeyboardKey key) const override;
			bool IsKeyUp(KeyboardKey key) override;

			bool IsMouseButtonPressed(MouseButton mouseButton) const override;
			bool IsMouseButtonDown(MouseButton mouseButton) const override;
			bool IsMouseButtonReleased(MouseButton mouseButton) const override;
			bool IsMouseButtonUp(MouseButton mouseButton) const override;

			bool IsGamepadButtonPressed(GamepadButton gamepadButton) const override;
			bool IsGamepadButtonDown(GamepadButton gamepadButton) const override;
			bool IsGamepadButtonReleased(GamepadButton gamepadButton) const override;
			bool IsGamepadButtonUp(GamepadButton gamepadButton) const override;

			InputState GetKeyState(KeyboardKey key) const override;
			InputState GetMouseButtonState(MouseButton mouseButton) const override;
			InputState GetGamepadButtonState(GamepadButton gamepadButton) const override;

			int GetMouseX() override;
			int GetMouseY() override;
			Vector2i GetMousePosition() override;

			float GetMouseDeltaX() override;
			float GetMouseDeltaY() override;
			Vector2f GetMouseDelta() override;

		protected:

			void PollInput() override;


		private:



		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<input::RaylibInputSubsystem>()
		{
			return "RaylibInputSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<input::RaylibInputSubsystem>()
		{
			return entt::hs(GetTypeString<input::RaylibInputSubsystem>().data());
		}

		template<>
		inline void RegisterType<input::RaylibInputSubsystem>()
		{
			auto meta = entt::meta<input::RaylibInputSubsystem>()
				.base<input::InputSubsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
