#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "GLFW/glfw3.h"

#include "puffin/core/subsystem.h"
#include "puffin/input/inputevent.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace input
	{
		struct InputAction
		{
			std::string name;
			int id = 0;
			std::vector<int> keys;
			KeyState state;
		};

		class InputSubsystem : public core::Subsystem
		{
		public:

			explicit InputSubsystem(const std::shared_ptr<core::Engine>& engine);
			~InputSubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			[[nodiscard]] core::SubsystemType GetType() const override;

			void ProcessInput() override;

			void AddAction(std::string name, int key);
			void AddAction(std::string name, std::vector<int> keys);
			[[nodiscard]] InputAction GetAction(std::string name) const;

			[[nodiscard]] bool JustPressed(const std::string& name) const;
			[[nodiscard]] bool Pressed(const std::string& name) const;
			[[nodiscard]] bool JustReleased(const std::string& name) const;
			[[nodiscard]] bool Released(const std::string& name) const;

			[[nodiscard]] double GetMouseXOffset() const;
			[[nodiscard]] double GetMouseYOffset() const;
			[[nodiscard]] double GetSensitivity() const;
			[[nodiscard]] bool GetCursorLocked() const;

		private:

			double mXPos, mYPos, mLastXPos, mLastYPos;
			bool mCursorLocked;
			double mSensitivity;
			bool mFirstMouse;

			int mNextID = 1;
			std::unordered_map<std::string, InputAction> mActions;
			GLFWwindow* mWindow;
		};
	}
}
