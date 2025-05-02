#pragma once

#include "puffin/input/inputsubsystem.h"

namespace puffin::input
{
	class InputSubsystemRL : public InputSubsystem
	{
	public:

		explicit InputSubsystemRL(const std::shared_ptr<core::Engine>& engine);
		~InputSubsystemRL() override = default;

		int GetMouseX() override;
		int GetMouseY() override;
		Vector2i GetMousePosition() override;

		double GetMouseDeltaX() override;
		double GetMouseDeltaY() override;
		Vector2f GetMouseDelta() override;

	protected:

		void PollInput() override;
		KeyState GetKeyState(KeyboardKey) override;

	private:



	};
}