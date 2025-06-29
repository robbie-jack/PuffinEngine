#include "core/subsystem.h"

#include "core/engine.h"
#include "core/subsystem_manager.h"

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

	void Subsystem::RegisterTypes()
	{
	}

	void Subsystem::Initialize(SubsystemManager* subsystemManager)
	{
	}

	void Subsystem::Deinitialize()
	{
	}

	SubsystemType Subsystem::GetType() const
	{
		return SubsystemType::Engine;
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

	void Subsystem::Update(double deltaTime)
	{
	}

	bool Subsystem::ShouldUpdate()
	{
		return false;
	}

	void Subsystem::FixedUpdate(double fixedTimeStep)
	{
	}

	bool Subsystem::ShouldFixedUpdate()
	{
		return false;
	}

	const std::string& Subsystem::GetName()
	{
		return mName;
	}
}
