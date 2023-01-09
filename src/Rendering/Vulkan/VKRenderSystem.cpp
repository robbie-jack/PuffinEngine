#include "Rendering/Vulkan/VKRenderSystem.hpp"

#include "Window/WindowSubsystem.hpp"
#include "Engine/Engine.hpp"

#include <iostream>

namespace Puffin::Rendering::VK
{
	void VKRenderSystem::Init()
	{
		InitVulkan();
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
}
