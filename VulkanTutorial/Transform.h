#pragma once

#include <glm/gtc/matrix_transform.hpp>

struct Transform
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	Transform(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 rot = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 sca = glm::vec3(0.0f, 0.0f, 0.0f))
	{
		position = pos;
		rotation = rot;
		scale = sca;
	}
};