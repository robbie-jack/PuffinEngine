#include "Rendering/BGFX/BGFXRenderSystem.hpp"

#include "bgfx/bgfx.h"

#include "Engine/Engine.hpp"
#include "Window/WindowSubsystem.hpp"

#include "GLFW/glfw3.h"

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD 
	#define GLFW_EXPOSE_NATIVE_X11
#else
	#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "GLFW/glfw3native.h"

namespace Puffin::Rendering
{
	void BGFXRenderSystem::Init()
	{
        bgfx::PlatformData pd;
        GLFWwindow* window = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();
        pd.nwh = glfwGetWin32Window(window);

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD 

#if ENTRY_CONFIG_USE_WAYLAND // examples entry options define
        pd.ndt = glfwGetWaylandDisplay();
#else 
        pd.ndt = glfwGetX11Display();
        pd.nwh = (void*)glfwGetX11Window(window);
#endif 

#elif BX_PLATFORM_OSX

        pd.ndt = NULL;

#elif BX_PLATFORM_WINDOWS 

        pd.ndt = NULL;
        pd.nwh = glfwGetWin32Window(window);

#endif // BX_PLATFORM_*
        bgfx::Init bgfxInit;

        // seems to default to vulkan which is fine by me!
        bgfxInit.type = bgfx::RendererType::Vulkan;
        //bgfxInit.type = bgfx::RendererType::OpenGL;

        bgfxInit.resolution.width = 1280;
        bgfxInit.resolution.height = 720;
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;

        // seems bgfx is bright enough to not use the active but unused integrated device!
        //bgfxInit.vendorId = BGFX_PCI_ID_NVIDIA; // just in case its selecting unused integrated device?

        bgfxInit.platformData = pd;
        bgfx::init(bgfxInit);
	}
}
