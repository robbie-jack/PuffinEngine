#pragma once

namespace puffin::rendering
{
	enum class RenderCommandType
	{
		BeginRendering = 0,
		EndRendering
	};

	class RenderCommand
	{
	public:

		explicit RenderCommand(RenderCommandType type) : mType(type) {}

		[[nodiscard]] RenderCommandType GetType() const { return mType; }

	private:

		RenderCommandType mType;

	};
}