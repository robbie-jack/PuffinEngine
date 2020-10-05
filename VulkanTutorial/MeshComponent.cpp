#include "MeshComponent.h"

namespace Puffin
{
	namespace Rendering
	{
		MeshComponent::MeshComponent()
		{
			type = ComponentType::MESH;
			name = "Mesh Component";
		}

		MeshComponent::~MeshComponent()
		{

		}
	}
}