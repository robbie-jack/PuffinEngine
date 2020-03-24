#include "Camera.h"

Camera::Camera()
{
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	lookat = glm::vec3(0.0f, 0.0f, 0.0f);
	direction = glm::vec3(0.0f, 0.0f, 0.0f);
	up = glm::vec3(0.0f, 0.0f, 0.0f);
	right = glm::vec3(0.0f, 0.0f, 0.0f);

	speed = 5.0f;
	yaw = 0.0;
	pitch = 0.0f;

	viewBufferObject.viewPos = position;
}

Camera::~Camera()
{

}

void Camera::Init(glm::vec3 position_, glm::vec3 direction_, glm::vec3 up_, float fov, float aspect, float near, float far)
{
	/*SetViewEye(eye);
	SetViewCentre(centre);
	SetViewUp(up);*/

	direction = direction_;

	// Set Position, LookAt and Up Vectors
	SetPosition(position_);
	SetLookAt(position + direction);
	SetUp(up_);

	// Calculate Perspective Projection
	SetPerspective(fov, aspect, near, far);

	// Calculate Right and Up vectors
	right = glm::normalize(glm::cross(up, direction));
	up = glm::cross(direction, right);

	yaw = -90.0f;
}

void Camera::Update(InputManager* inputManager, float delta_time)
{
	// Camera Movement
	if (inputManager->GetAction("CamMoveLeft").state == HELD)
	{
		position += speed * right * delta_time;
	}
	else if (inputManager->GetAction("CamMoveRight").state == HELD)
	{
		position -= speed * right * delta_time;
	}

	if (inputManager->GetAction("CamMoveForward").state == HELD)
	{
		position += speed * direction * delta_time;
	}
	else if (inputManager->GetAction("CamMoveBackward").state == HELD)
	{
		position -= speed * direction * delta_time;
	}

	if (inputManager->GetAction("CamMoveUp").state == HELD)
	{
		position += speed * up * delta_time;
	}
	else if (inputManager->GetAction("CamMoveDown").state == HELD)
	{
		position -= speed * up * delta_time;
	}

	// Mouse Rotation
	if (inputManager->IsCursorLocked())
	{
		yaw += inputManager->GetMouseXOffset();
		pitch -= inputManager->GetMouseYOffset();

		if (pitch > 89.0f)
			pitch = 89.0f;

		if (pitch < -89.0f)
			pitch = -89.0f;

		// Calculate Direction vector from yaw and pitch of camera
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction = glm::normalize(direction);
	}

	// Calculate Right, Up and LookAt vectors
	right = glm::normalize(glm::cross(up, direction));
	lookat = position + direction;

	UpdateViewMatrix();
}

void Camera::SetPerspective(float fov_, float aspect_, float zNear_, float zFar_)
{
	fov = fov_;
	zNear = zNear_;
	zFar = zFar_;

	matrices.perspective = glm::perspective(glm::radians(fov), aspect_, zNear, zFar);
}

void Camera::UpdateViewMatrix()
{
	/*glm::mat4 rotM = glm::mat4(1.0f);
	glm::mat4 transM;

	rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(1.0f, 1.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(1.0f, 0.0f, 1.0f));

	transM = glm::translate(glm::mat4(1.0f), position);

	matrices.view = transM * rotM;*/

	matrices.view = glm::lookAt(position, position + direction, up);
}