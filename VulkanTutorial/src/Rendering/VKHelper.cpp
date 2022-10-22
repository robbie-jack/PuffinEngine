#include "VKHelper.h"

#include <vector>

namespace Puffin
{
	namespace Rendering
	{
		namespace VKHelper
		{
			VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat)
			{
				// Since all depth formats may be optional, we need to find a suitable depth format to use
				// Start with the highest precision packed format
				std::vector<VkFormat> depthFormats = {
					VK_FORMAT_D32_SFLOAT_S8_UINT,
					VK_FORMAT_D32_SFLOAT,
					VK_FORMAT_D24_UNORM_S8_UINT,
					VK_FORMAT_D16_UNORM_S8_UINT,
					VK_FORMAT_D16_UNORM
				};

				for (auto& format : depthFormats)
				{
					VkFormatProperties formatProps;
					vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

					if (formatProps.linearTilingFeatures & !VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT & !VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
					{
						continue;
					}

					// Format must support depth stencil attachment for optimal tiling
					if (formatProps.optimalTilingFeatures & !VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
					{
						continue;
					}

					*depthFormat = format;
					return true;
				}

				return false;
			}
		}
	}
}