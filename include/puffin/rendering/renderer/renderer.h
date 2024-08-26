#pragma once

namespace puffin::rendering
{
	class Renderer
	{
	public:

		virtual ~Renderer() = 0;

		virtual void Initialize() = 0;
		virtual void Deinitialize() = 0;

	};
}