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
		InitSwapchain();
		InitCommands();
	}

	void VKRenderSystem::Update()
	{
		
	}

	void VKRenderSystem::Cleanup()
	{
		m_device.waitIdle();

		if (m_isInitialized)
		{
			m_deletionQueue.Flush();

			m_isInitialized = false;
		}
	}

	void VKRenderSystem::InitVulkan()
	{
		GLFWwindow* glfwWindow = m_engine->GetSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

		// Create Vulkan Instance
		int width, height;
		glfwGetWindowSize(glfwWindow, &width, &height);
		m_windowSize.width = static_cast<unsigned>(width);
		m_windowSize.height = static_cast<unsigned>(height);

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

		// Get Graphics Queue
		m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroy();
			m_instance.destroySurfaceKHR(m_surface);
			vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
			m_instance.destroy();
		});

		m_isInitialized = true;
	}

	void VKRenderSystem::InitSwapchain()
	{
		vkb::SwapchainBuilder swapchainBuilder { m_physicalDevice, m_device, m_surface};

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.use_default_format_selection()
			// Vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(m_windowSize.width, m_windowSize.height)
			.build()
			.value();

		m_swapchain = vkbSwapchain.swapchain;
		m_swapchainImageFormat = static_cast<vk::Format>(vkbSwapchain.image_format);

		std::vector<VkImage> images = vkbSwapchain.get_images().value();
		std::vector<VkImageView> imageViews = vkbSwapchain.get_image_views().value();

		m_swapchainImages.resize(images.size());
		m_swapchainImageViews.resize(imageViews.size());

		for (int i = 0; i < images.size(); i++)
		{
			m_swapchainImages[i] = static_cast<vk::Image>(images[i]);
			m_swapchainImageViews[i] = static_cast<vk::ImageView>(imageViews[i]);
		}

		images.clear();
		imageViews.clear();

		m_deletionQueue.PushFunction([=]()
		{
			for (int i = 0; i < m_swapchainImageViews.size(); i++)
			{
				m_device.destroyImageView(m_swapchainImageViews[i]);
			}

			m_device.destroySwapchainKHR(m_swapchain);
		});
	}

	void VKRenderSystem::InitCommands()
	{
		vk::CommandPoolCreateInfo commandPoolInfo = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphicsQueueFamily };
		m_commandPool = m_device.createCommandPool(commandPoolInfo);

		vk::CommandBufferAllocateInfo commandBufferInfo = { m_commandPool, vk::CommandBufferLevel::ePrimary, 1 };
		m_mainCommandBuffer = m_device.allocateCommandBuffers(commandBufferInfo)[0];

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyCommandPool(m_commandPool);
		});
	}

	vk::UniquePipeline VKRenderSystem::BuildTrianglePipeline()
	{
		/*vku::PipelineLayoutMaker plm{};
		vk::UniquePipelineLayout pl = plm.createUnique(m_device);*/

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
}
