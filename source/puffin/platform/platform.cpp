#include <utility>

#include "puffin/platform/platform.h"

namespace puffin::core
{
	Platform::Platform(std::shared_ptr<Engine> engine)
		: mEngine(std::move(engine))
	{
		mName = "None";
	}
}
