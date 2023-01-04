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

#include "Assets/AssetRegistry.h"
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

        /*m_vsh = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\vs_cubes.bin");
        m_fsh = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\fs_cubes.bin");*/

        m_vsh = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\forward_shading\\vs_forward_shading.bin");
        m_fsh = LoadShader("C:\\Projects\\PuffinEngine\\bin\\spirv\\forward_shading\\fs_forward_shading.bin");

        m_program = bgfx::createProgram(m_vsh, m_fsh, true);
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

        // Setup Transform
        float mtx[16];
        bx::mtxRotateXY(mtx, m_frameCounter * 0.01f, m_frameCounter * 0.01f);
        bgfx::setTransform(mtx);

        // Set Vertex/Index Buffers
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);

        // Submit Program
        bgfx::submit(0, m_program);

        // Advance to next frame
		bgfx::frame();

        m_frameCounter++;
	}

	void BGFXRenderSystem::Cleanup()
	{
        bgfx::destroy(m_ibh);
		bgfx::destroy(m_vbh);

        bgfx::destroy(m_program);

		bgfx::shutdown();
	}
}
