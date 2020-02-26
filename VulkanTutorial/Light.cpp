#include "Light.h"

Light::Light()
{

}

Light::~Light()
{

}

void Light::InitLight(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse)
{
	lightUniformBuffer.position = position;
	lightUniformBuffer.ambientColor = ambient;
	lightUniformBuffer.diffuseColor = diffuse;
}