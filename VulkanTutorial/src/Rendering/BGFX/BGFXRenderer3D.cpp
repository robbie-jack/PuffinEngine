#include "Rendering/BGFX/BGFXRenderer3D.hpp"

#include "bgfx/bgfx/bgfx.h"
#include "bgfx/bgfx/platform.h"
#include "bgfx/bx/math.h"

#include "GLFW/glfw3.h"

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD 
#define GLFW_EXPOSE_NATIVE_X11
#else
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

namespace Puffin::Rendering::BGFX
{
	bgfx::PlatformData pd;


}