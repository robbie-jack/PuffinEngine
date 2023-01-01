#include "Rendering/BGFX/BGFXRenderSystem.hpp"

#include "bgfx/bgfx.h"

#include "Engine/Engine.hpp"
#include "Window/WindowSubsystem.hpp"

#include "GLFW/glfw3.h"

#if PFN_PLATFORM_WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#else
X_PLATFORM_LINUX || BX_PLATFORM_BSD
	#define GLFW_EXPOSE_NATIVE_X11
#endif

#include "GLFW/glfw3native.h"

namespace Puffin::Rendering
{
	void BGFXRenderSystem::Init()
	{
        bgfx::PlatformData pd;
        GLFWwindow* window = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

#if PFN_PLATFORM_WIN32

        pd.ndt = NULL;
        pd.nwh = glfwGetWin32Window(window);

#else

        pd.ndt = glfwGetX11Display();
        pd.nwh = (void*)glfwGetX11Window(window);

        // Set Wayland instead of X11
        // pd.ndt = glfwGetWaylandDisplay();

#endif

        bgfx::Init bgfxInit;

        // Set Renderer API to Vulkan
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
