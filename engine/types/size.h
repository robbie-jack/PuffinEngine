#pragma once

namespace puffin
{
	struct Size
	{
		uint32_t width = 0;
		uint32_t height = 0;

		bool operator==(const Size other) const
		{
			return width == other.width && height == other.height;
		}

		bool operator!=(const Size other) const
		{
			return width != other.width || height != other.height;
		}
	};
}