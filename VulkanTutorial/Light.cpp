#include "Light.h"

namespace Puffin
{
	namespace Rendering
	{
		Light::Light()
		{

		}

		Light::~Light()
		{

		}

		void Light::InitLight(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, float specular, int shininess)
		{
			lightUniformBuffer.position = position;
			lightUniformBuffer.ambientColor = ambient;
			lightUniformBuffer.diffuseColor = diffuse;
			lightUniformBuffer.specularStrength = specular;
			lightUniformBuffer.shininess = shininess;
		}
	}
}