#include "subsystem/editor_subsystem.h"

namespace puffin::core
{
	EditorSubsystem::EditorSubsystem(std::shared_ptr<Engine> engine)
		: EngineSubsystem(engine)
	{
	}

	EditorSubsystem::~EditorSubsystem()
	{
	}
}
