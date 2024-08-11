#include "puffin/core/subsystem.h"

#include "puffin/core/engine.h"
#include "puffin/core/subsystem_manager.h"

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

	void Subsystem::initialize(SubsystemManager* subsystem_manager)
	{

	}

	void Subsystem::deinitialize()
	{

	}

	SubsystemType Subsystem::type() const
	{
		return SubsystemType::Engine;
	}

	void Subsystem::begin_play()
	{

	}

	void Subsystem::end_play()
	{

	}

	void Subsystem::update(double delta_time)
	{
		
	}

	bool Subsystem::should_update()
	{
		return false;
	}

	void Subsystem::fixed_update(double fixed_time)
	{
	}

	bool Subsystem::should_fixed_update()
	{
		return false;
	}

	void Subsystem::process_input()
	{
	}

	double Subsystem::wait_for_last_presentation_and_sample_time()
	{
		return 0.0;
	}

	void Subsystem::render(double delta_time)
	{
	}
}
