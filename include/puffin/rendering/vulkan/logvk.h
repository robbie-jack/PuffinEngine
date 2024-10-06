#pragma once

#include <iostream>
#include <unordered_map>

#include "vulkan/vulkan.hpp"

namespace puffin::rendering
{
	inline std::unordered_map<vk::Result, std::string> gResultToString =
	{
		{ vk::Result::eSuccess, "Success" },
		{ vk::Result::eTimeout, "Timeout" },
	};

	inline vk::Result LogResultVK(const std::string& methodName, vk::Result result)
	{
		if (result != vk::Result::eSuccess)
		{
			if (gResultToString.find(result) != gResultToString.end())
			{
				std::cout << "Method " << methodName << " completed with result " << gResultToString.at(result) << std::endl;
				return result;
			}

			std::cout << "Method " << methodName.c_str() << " completed with result " << result << std::endl;
		}

		return result;
	}
}