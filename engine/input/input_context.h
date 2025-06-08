#pragma once

#include <string>
#include <unordered_map>

#include "input/input_types.h"

namespace puffin::input
{
	class InputContext
	{
	public:

		explicit InputContext(std::string name);
		~InputContext() = default;

		InputAction& AddAction(const std::string& name);
		void AddAction(const std::string& name, const InputAction& action);
		void RemoveAction(const std::string& name);
		InputAction& GetAction(const std::string& name);
		[[nodiscard]] const InputAction& GetAction(const std::string& name) const;
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
		bool mBlockInput;
		std::unordered_map<std::string, InputAction> mActions;

	};
}
