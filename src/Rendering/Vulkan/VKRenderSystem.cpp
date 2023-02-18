#include "Rendering/Vulkan/VKRenderSystem.hpp"

#define VMA_IMPLEMENTATION

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include "vk_mem_alloc.hpp"
#include "VkBootstrap.h"

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
		//m_window.draw(m_device, m_framework.graphicsQueue());
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
		im.extensionValidation();

		vku::DeviceMaker dm{};
		dm.defaultLayers();

		//// Initialize VKU Framework
		//m_framework = vku::Framework(im, dm);
		//if (!m_framework.ok())
		//{
		//	std::cout << "Framework creation failed" << std::endl;
		//	return;
		//}

		//m_device = m_framework.device();

		//// Create VKU Window
		//m_window = vku::Window(
		//	m_framework.instance(),
		//	m_device,
		//	m_framework.physicalDevice(),
		//	m_framework.graphicsQueueFamilyIndex(),
		//	glfwWindow
		//);

		//if (!m_window.ok())
		//{
		//	std::cout << "Framework creation failed" << std::endl;
		//	return;
		//}

		//m_window.clearColorValue() = { .0f, .7f, .9f, 1.0f };

		// Create Vulkan Instance
		vkb::InstanceBuilder instBuilder;

		auto instRet = instBuilder.set_app_name("Puffin Engine")
			.request_validation_layers(enableValidationLayers)
			.require_api_version(1, 3, 0)
			.use_default_debug_messenger()
			.build();

		vkb::Instance vkbInst = instRet.value();

		// Store Instance/Debug Messenger
		m_instance = vkbInst.instance;
		m_debugMessenger = vkbInst.debug_messenger;

		// Get Window Surface
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(m_instance, glfwWindow, nullptr, &surface);
		m_surface = surface;

		// Select GPU
		vkb::PhysicalDeviceSelector selector { vkbInst };
		vkb::PhysicalDevice physDevice = selector
			.set_minimum_version(1, 3)
			.set_surface(m_surface)
			.select()
			.value();

		// Create Vulkan Device
		vkb::DeviceBuilder deviceBuilder { physDevice };
		vkb::Device vkbDevice = deviceBuilder.build().value();

		m_device = vkbDevice.device;
		m_physicalDevice = physDevice.physical_device;

		m_isInitialized = true;
	}

	void VKRenderSystem::InitSwapchain()
	{

	}

	vk::UniquePipeline VKRenderSystem::BuildTrianglePipeline()
	{
		vku::PipelineLayoutMaker plm{};
		vk::UniquePipelineLayout pl = plm.createUnique(m_device);

		/*vku::PipelineMaker pm{ m_window.width(), m_window.height() };
		return pm
			.shader(vk::ShaderStageFlagBits::eVertex, m_vertMod)
			.shader(vk::ShaderStageFlagBits::eFragment, m_fragMod)
			.vertexBinding(0, sizeof(VertexPC32))
			.vertexAttribute(0, 0, vk::Format::eR32G32Sfloat, offsetof(VertexPC32, pos))
			.vertexAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPC32, color))
			.createUnique(m_device, m_framework.pipelineCache(), *pl, m_window.renderPass());*/

		return vk::UniquePipeline();
	}

	void VKRenderSystem::InitTriangleRender()
	{
		//vku::HostVertexBuffer buffer(m_device, m_framework.memprops(), s_triangleVertices);

		//m_vertMod = vku::ShaderModule{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\hellotriangle\\hello_triangle_vs.spv" };
		//m_fragMod = vku::ShaderModule{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\hellotriangle\\hello_triangle_fs.spv" };

		//// Make a pipeline to use the vertex format and shaders.
		//m_triPipeline = BuildTrianglePipeline();

		//m_window.setStaticCommands([&](vk::CommandBuffer cb, int imageIndex, vk::RenderPassBeginInfo& rpbi)
		//{
		//	static auto ww = m_window.width();
		//	static auto wh = m_window.height();

		//	if (ww != m_window.width() || wh != m_window.height()) 
		//	{
		//		ww = m_window.width();
		//		wh = m_window.height();

		//		// Make a pipeline to use the vertex format and shaders.
		//		vku::PipelineMaker pm{ m_window.width(), m_window.height() };

		//		m_triPipeline = BuildTrianglePipeline();
		//	}

		//	vk::CommandBufferBeginInfo cbbi{};
		//	cb.begin(cbbi);
		//	cb.beginRenderPass(rpbi, vk::SubpassContents::eInline);
		//	cb.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_triPipeline);
		//	cb.bindVertexBuffers(0, m_triVertexBuffer.buffer(), vk::DeviceSize(0));
		//	cb.draw(s_triangleVertices.size(), 1, 0, 0);
		//	cb.endRenderPass();
		//	cb.end();
		//});
	}
}
