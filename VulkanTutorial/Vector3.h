#include <glm/gtc/matrix_transform.hpp>
#include "reactphysics3d/reactphysics3d.h"
#include <math.h>

namespace Puffin
{
	struct Vector3
	{
		// Variables
		float x;
		float y;
		float z;

		// Constructors
		Vector3()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}

		Vector3(float x_, float y_, float z_)
		{
			x = x_;
			y = y_;
			z = z_;
		}

		// Fill X/Y/Z Components with Val
		Vector3(float val)
		{
			x = val;
			y = val;
			z = val;
		}

		// Operator Overrides
		// Operator=
		void operator=(glm::vec3 vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		void operator=(rp3d::Vector3 vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		// Operator+=
		void operator+=(glm::vec3 vec)
		{
			x += vec.x;
			y += vec.y;
			z += vec.z;
		}

		void operator+=(rp3d::Vector3 vec)
		{
			x += vec.x;
			y += vec.y;
			z += vec.z;
		}

		// Operator-=
		void operator-=(glm::vec3 vec)
		{
			x -= vec.x;
			y -= vec.y;
			z -= vec.z;
		}

		void operator-=(rp3d::Vector3 vec)
		{
			x -= vec.x;
			y -= vec.y;
			z -= vec.z;
		}

		// Operator+
		Vector3 operator+(Vector3 vec)
		{
			Vector3 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			vector.z = z + vec.z;
			return vector;
		}

		Vector3 operator+ (glm::vec3 vec)
		{
			Vector3 vector;
			vector = x + vec.x;
			vector = y + vec.y;
			vector = z + vec.z;
			return vector;
		}

		Vector3 operator+ (rp3d::Vector3 vec)
		{
			Vector3 vector;
			vector = x + vec.x;
			vector = y + vec.y;
			vector = z + vec.z;
			return vector;
		}

		// Operator-
		Vector3 operator-(Vector3 vec)
		{
			Vector3 vector;
			vector.x = x - vec.x;
			vector.y = y - vec.y;
			vector.z = z - vec.z;
			return vector;
		}

		Vector3 operator- (glm::vec3 vec)
		{
			Vector3 vector;
			vector = x - vec.x;
			vector = y - vec.y;
			vector = z - vec.z;
			return vector;
		}

		Vector3 operator- (rp3d::Vector3 vec)
		{
			Vector3 vector;
			vector = x - vec.x;
			vector = y - vec.y;
			vector = z - vec.z;
			return vector;
		}

		// Functions
		float Dot(Vector3 vec)
		{
			return (x * vec.x) + (y * vec.y) + (z * vec.z);
		}

		Vector3 Cross(Vector3 vec)
		{
			Vector3 cross;
			cross.x = y * vec.z - z * vec.y;
			cross.y = x * vec.z - z * vec.x;
			cross.z = x * vec.y - y * vec.x;
			return cross;
		}

		float Length()
		{
			return sqrtf(x * x + y * y + z * z);
		}

		Vector3 Normalised()
		{
			Vector3 vector;
			float length = Length();

			vector.x / length;
		}

	};

	struct Vector3Double
	{
		double x;
		double y;
		double z;
	};
}