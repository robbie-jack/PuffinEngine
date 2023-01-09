#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX

#include "vulkan/vulkan.hpp"
#include "vku/vku.hpp"
#include "vku/vku_framework.hpp"

#include "ECS/System.hpp"

namespace Puffin::Rendering::VK
{
	// Vulkan Rendering System
	class VKRenderSystem : public ECS::System
	{
	public:

		VKRenderSystem()
		{
			m_systemInfo.name = "VKRenderSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::Render;
		}

		void Init() override;
		void PreStart() override {}
		void Start() override {}
		void Update() override {}
		void Stop() override {}
		void Cleanup() override {}

	private:

		// Initialization Members
		vku::Framework m_framework;
		vk::Device m_device;
		vku::Window m_window;

		// Indicated initialization completed without any failures
		bool m_isInitialized = false;

		void InitVulkan();

	};
}