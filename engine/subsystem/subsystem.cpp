#include "subsystem/subsystem.h"

#include "core/engine.h"
#include "subsystem/subsystem_manager.h"

namespace puffin::core
{
	Subsystem::Subsystem(std::shared_ptr<Engine> engine) :
		mEngine(std::move(engine))
	{
	}

	Subsystem::~Subsystem()
	{
		mEngine = nullptr;
	}

	void Subsystem::PreInitialize()
	{
	}

	void Subsystem::Initialize(SubsystemManager* subsystemManager)
	{
		mInitialized = true;
	}

	void Subsystem::PostInitialize()
	{
	}

	void Subsystem::Deinitialize()
	{
		mInitialized = false;
	}

	void Subsystem::PostSceneLoad()
	{
	}

	void Subsystem::BeginPlay()
	{
	}

	void Subsystem::EndPlay()
	{
	}

	std::string_view Subsystem::GetName() const
	{
		return reflection::GetTypeString<Subsystem>();
	}
}
