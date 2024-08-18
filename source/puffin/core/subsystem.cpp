#include "puffin/core/subsystem.h"

#include "puffin/core/engine.h"
#include "puffin/core/subsystemmanager.h"

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

	void Subsystem::RegisterSubsystems()
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

	void Subsystem::ProcessInput()
	{
	}

	double Subsystem::WaitForLastPresentationAndSampleTime()
	{
		return 0.0;
	}

	void Subsystem::Render(double deltaTime)
	{
	}

	const std::string& Subsystem::GetName()
	{
		return mName;
	}
}
