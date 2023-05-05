#include "Engine/EnkiTSSubsystem.hpp"

#include "Engine/Engine.hpp"

namespace puffin::core
{
	void EnkiTSSubsystem::setupCallbacks()
	{
		mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "EnkiTSSubsystem: Init", 50);
	}
}