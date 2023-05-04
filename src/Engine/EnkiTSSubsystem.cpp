#include "Engine/EnkiTSSubsystem.hpp"

#include "Engine/Engine.hpp"

namespace Puffin::Core
{
	void EnkiTSSubsystem::SetupCallbacks()
	{
		m_engine->registerCallback(Core::ExecutionStage::init, [&]() { Init(); }, "EnkiTSSubsystem: Init", 50);
	}
}