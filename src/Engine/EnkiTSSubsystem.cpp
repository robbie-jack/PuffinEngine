#include "Engine/EnkiTSSubsystem.hpp"

#include "Engine/Engine.hpp"

namespace puffin::core
{
	void EnkiTSSubsystem::SetupCallbacks()
	{
		m_engine->registerCallback(core::ExecutionStage::Init, [&]() { Init(); }, "EnkiTSSubsystem: Init", 50);
	}
}