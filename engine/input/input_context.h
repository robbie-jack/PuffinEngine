#pragma once

#include <string>
#include <unordered_map>

#include "input/input_types.h"
#include "input/input_event.h"
#include "core/signal.h"

namespace puffin::input
{
	class InputContext
	{
	public:

		explicit InputContext(std::string name);
		~InputContext();

		InputAction& AddAction(const std::string& name);
		void AddAction(const std::string& name, const InputAction& action);
		void RemoveAction(const std::string& name);
		InputAction& GetAction(const std::string& name);
		[[nodiscard]] const InputAction& GetAction(const std::string& name) const;
		[[nodiscard]] Signal<InputEvent>* GetActionSignal(const std::string& name);
		[[nodiscard]] std::unordered_map<std::string, InputAction>& GetActions();

		bool HasAction(const std::string& name);
		[[nodiscard]] bool IsActionPressed(const std::string& name) const;
		[[nodiscard]] bool IsActionDown(const std::string& name) const;
		[[nodiscard]] bool IsActionReleased(const std::string& name) const;
		[[nodiscard]] bool IsActionUp(const std::string& name) const;

		void SetBlockInput(bool blockInput);
		[[nodiscard]] bool GetBlockInput() const;

	private:

		std::string mName;
		bool mBlockInput = false;
		std::unordered_map<std::string, InputAction> mActions;
		std::unordered_map<std::string, Signal<InputEvent>*> mActionSignals;

	};
}
