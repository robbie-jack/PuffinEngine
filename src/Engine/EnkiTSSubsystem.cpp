#include "Engine/EnkiTSSubsystem.hpp"

#include "Engine/Engine.hpp"

namespace Puffin::Core
{
	void EnkiTSSubsystem::SetupCallbacks()
	{
		m_engine->RegisterCallback(Core::ExecutionStage::Init, [&]() { Init(); }, "EnkiTSSubsystem: Init", 50);
	}
}