#include "Rendering/Vulkan/VKRenderSystem.hpp"

//#define VK_USE_PLATFORM_WIN32_KHR
//#define VMA_IMPLEMENTATION

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

//#include <vulkan/vulkan.hpp>
//#include "vk_mem_alloc.hpp"
//#include "vku/vku_framework.hpp"

#include "Window/WindowSubsystem.hpp"
#include "Engine/Engine.hpp"

#include <iostream>

namespace Puffin::Rendering::VK
{
	void VKRenderSystem::Init()
	{
		InitVulkan();
		InitTriangleRender();
	}

	void VKRenderSystem::Update()
	{
		m_window.draw(m_device, m_framework.graphicsQueue());
	}

	void VKRenderSystem::Cleanup()
	{
		m_device.waitIdle();
	}

	void VKRenderSystem::InitVulkan()
	{
		GLFWwindow* glfwWindow = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

		// Initialize makers
		vku::InstanceMaker im{};
		im.defaultLayers();
		vku::DeviceMaker dm{};
		dm.defaultLayers();

		// Initialize VKU Framework
		m_framework = vku::Framework(im, dm);
		if (!m_framework.ok())
		{
			std::cout << "Framework creation failed" << std::endl;
			return;
		}

		m_device = m_framework.device();

		// Create VKU Window
		m_window = vku::Window(
			m_framework.instance(),
			m_device,
			m_framework.physicalDevice(),
			m_framework.graphicsQueueFamilyIndex(),
			glfwWindow
		);

		if (!m_window.ok())
		{
			std::cout << "Framework creation failed" << std::endl;
			return;
		}

		m_isInitialized = true;
	}

	void VKRenderSystem::InitTriangleRender()
	{
		vku::HostVertexBuffer buffer(m_device, m_framework.memprops(), s_triangleVertices);

		vku::ShaderModule vert{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\hellotriangle\\hello_triangle_vs.spv" };
		vku::ShaderModule frag{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\hellotriangle\\hello_triangle_fs.spv" };

		vku::PipelineLayoutMaker plm{};
		auto pipelineLayout = plm.createUnique(m_device);

		auto buildPipeline = [&]()
		{
			// Make a pipeline to use the vertex format and shaders.
			vku::PipelineMaker pm{ m_window.width(), m_window.height() };
			return pm
				.shader(vk::ShaderStageFlagBits::eVertex, vert)
				.shader(vk::ShaderStageFlagBits::eFragment, frag)
				.vertexBinding(0, sizeof(VertexPC32))
				.vertexAttribute(0, 0, vk::Format::eR32G32Sfloat, offsetof(VertexPC32, pos))
				.vertexAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPC32, color))
				.createUnique(m_device, m_framework.pipelineCache(), *pipelineLayout, m_window.renderPass());
		};
		m_triPipeline = buildPipeline();

		m_window.setStaticCommands([this, &buildPipeline](vk::CommandBuffer cb, int imageIndex, vk::RenderPassBeginInfo& rpbi)
		{
			static auto ww = m_window.width();
			static auto wh = m_window.height();

			if (ww != m_window.width() || wh != m_window.height()) {
				ww = m_window.width();
				wh = m_window.height();
				m_triPipeline = buildPipeline();
			}

			vk::CommandBufferBeginInfo cbbi{};
			cb.begin(cbbi);
			cb.beginRenderPass(rpbi, vk::SubpassContents::eInline);
			cb.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_triPipeline);
			cb.bindVertexBuffers(0, m_triVertexBuffer.buffer(), vk::DeviceSize(0));
			cb.draw(s_triangleVertices.size(), 1, 0, 0);
			cb.endRenderPass();
			cb.end();
		});
	}
}
