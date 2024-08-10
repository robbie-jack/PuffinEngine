#pragma once

#include "puffin/core/engine_subsystem.h"

namespace puffin::editor
{
	/*
	 * Subsystem which is only created when editor is active
	 */
	class EditorSubsystem : public core::EngineSubsystem
	{
	public:

		EditorSubsystem(std::shared_ptr<core::Engine> engine);
		~EditorSubsystem() override = default;

	};
}