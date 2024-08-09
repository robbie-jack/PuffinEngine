#include "puffin/core/subsystem.h"
#include "puffin/core/engine.h"

namespace puffin::core
{
	Subsystem::Subsystem(std::shared_ptr<Engine> engine) : m_engine(std::move(engine))
	{

	}

	Subsystem::~Subsystem()
	{
		m_engine = nullptr;
	}

	void Subsystem::initialize()
	{

	}

	void Subsystem::deinitialize()
	{

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
}
