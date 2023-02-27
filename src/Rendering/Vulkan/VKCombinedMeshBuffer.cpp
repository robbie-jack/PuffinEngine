#include "Rendering/Vulkan/VKCombinedMeshBuffer.hpp"

namespace Puffin::Rendering::VK
{
	bool CombinedMeshBuffer::AddMesh(std::shared_ptr<Assets::StaticMeshAsset> staticMesh)
	{
		if (staticMesh && staticMesh->Load())
		{
			if (m_vertexSize != staticMesh->GetVertexSize())
				return false;



			return true;
		}
		else
		{
			return false;
		}
	}
}
