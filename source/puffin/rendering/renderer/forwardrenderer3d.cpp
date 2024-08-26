#include "puffin/rendering/renderer/forwardrenderer3d.h"

#include "puffin/rendering/rendersubsystem.h"
#include "puffin/rendering/rendergraph/rendergraph.h"
#include "puffin/rendering/rendergraph/renderpass.h"

namespace puffin::rendering
{
	ForwardRenderer3D::ForwardRenderer3D(RenderSubsystem* renderSubsystem)
		: Renderer(renderSubsystem)
	{

	}

	void ForwardRenderer3D::Initialize()
	{

	}

	void ForwardRenderer3D::Deinitialize()
	{

	}

	void ForwardRenderer3D::SetupRenderPasses()
	{
		auto& renderGraph = mRenderSubsystem->GetRenderGraph();

		auto& forward = renderGraph.AddRenderPass("forward", RenderPassType::Graphics);
	}

	void ForwardRenderer3D::PrepareRenderCommands()
	{
		auto renderGraph = mRenderSubsystem->GetRenderGraph();

		auto forward = renderGraph.GetRenderPass("forward");
	}
}
