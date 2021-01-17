#pragma once

#include <vulkan/vulkan.h>

namespace Puffin
{
	namespace Rendering
	{
		namespace VKDebug
		{
			static PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;

			void Setup(VkInstance instance)
			{
				pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
			}

			void SetObjectName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, const char* name)
			{
				if (pfnSetDebugUtilsObjectNameEXT)
				{
					VkDebugUtilsObjectNameInfoEXT nameInfo = {};
					nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
					nameInfo.objectType = objectType;
					nameInfo.objectHandle = objectHandle;
					nameInfo.pObjectName = name;

					pfnSetDebugUtilsObjectNameEXT(device, &nameInfo);
				}
			}
		}
	}
}