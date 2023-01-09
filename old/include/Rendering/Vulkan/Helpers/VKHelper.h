#pragma once

#include "vulkan/vulkan.h"

namespace Puffin
{
	namespace Rendering
	{
		namespace VKHelper
		{
			VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat);
		}
	}
}
