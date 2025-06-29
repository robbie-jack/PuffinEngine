#include "subsystem/subsystem.h"

#include "core/engine.h"
#include "subsystem/subsystem_manager.h"

namespace puffin::core
{
	Subsystem::Subsystem(std::shared_ptr<Engine> engine) :
		m_engine(std::move(engine))
	{
	}

	Subsystem::~Subsystem()
	{
		m_engine = nullptr;
	}

	void Subsystem::PreInitialize(core::SubsystemManager* subsystemManager)
	{
	}

	void Subsystem::Initialize()
	{
	}

	void Subsystem::PostInitialize()
	{
	}

	void Subsystem::Deinitialize()
	{
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
