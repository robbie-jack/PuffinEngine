#pragma once

#include "puffin/core/subsystem.h"

namespace puffin::editor
{
	/*
	 * Subsystem which is only created when editor is active
	 */
	class EditorSubsystem : public core::Subsystem
	{
	public:

		EditorSubsystem(std::shared_ptr<core::Engine> engine);
		~EditorSubsystem() override = default;

	};
}