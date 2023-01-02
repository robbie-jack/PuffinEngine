#include "Rendering/BGFX/BGFXRenderSystem.hpp"

#include "bgfx/bgfx.h"
#include "bx/math.h"

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

namespace Puffin::Rendering::BGFX
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

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        bgfxInit.resolution.width = static_cast<uint32_t>(width);
        bgfxInit.resolution.height = static_cast<uint32_t>(height);
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
        bgfxInit.platformData = pd;
        bgfx::init(bgfxInit);

        bgfx::setDebug(BGFX_DEBUG_NONE);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x00B5E2FF, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height));

        // Create Static Vertex Buffer
        m_vbh = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), s_layoutVertexPC32);

        // Create Static Index Buffer
        m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));
	}

	void BGFXRenderSystem::Update()
	{
        GLFWwindow* window = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Dummy draw call to make sure view 0 is cleared
		bgfx::touch(0);

        // Setup View/Projection Matrices
        const bx::Vec3 at = {0.0f, 0.0f, 0.0f};
        const bx::Vec3 eye = { 0.0f, 0.0f, -5.0f };

        float view[16];
        bx::mtxLookAt(view, eye, at);

        float proj[16];
        bx::mtxProj(proj, 60.0f, width / height, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, view, proj);

        // Set Vertex/Index Buffers
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);

        // Advance to next frame
		bgfx::frame();

        m_frameCounter++;
	}

	void BGFXRenderSystem::Cleanup()
	{
        bgfx::destroy(m_ibh);
		bgfx::destroy(m_vbh);

		bgfx::shutdown();
	}
}
