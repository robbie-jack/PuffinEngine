#include "platform.h"

#include <utility>

namespace puffin::core
{
	Platform::Platform(std::shared_ptr<Engine> engine)
		: mEngine(std::move(engine))
	{
		mName = "None";
	}
}
