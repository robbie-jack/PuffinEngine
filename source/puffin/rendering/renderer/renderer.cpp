#include "puffin/rendering/renderer/renderer.h"

namespace puffin::rendering
{
	Renderer::Renderer(RenderSubsystem* renderSubsystem)
		: mRenderSubsystem(renderSubsystem)
	{
	}

	Renderer::~Renderer()
	{
		mRenderSubsystem = nullptr;
	}
}
