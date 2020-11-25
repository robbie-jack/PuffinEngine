#include <glm/gtc/matrix_transform.hpp>
#include "reactphysics3d/reactphysics3d.h"
#include "btBulletDynamicsCommon.h"
#include <math.h>

namespace Puffin
{
	struct Vector2
	{
		float x, y;

		// Constructors
		Vector2()
		{
			x = 0.0f;
			y = 0.0f;
		}

		Vector2(float x_, float y_)
		{
			x = x_;
			y = y_;
		}

		// Fill X/Y/Z Components with Val
		Vector2(float val)
		{
			x = val;
			y = val;
		}

		Vector2(glm::vec2 vec)
		{
			x = vec.x;
			y = vec.y;
		}

		Vector2(rp3d::Vector2 vec)
		{
			x = vec.x;
			y = vec.y;
		}

		// Operator Overrides

		// Operator Conversion
		operator glm::vec2() const
		{
			glm::vec2 vec;
			vec.x = x;
			vec.y = y;
			return vec;
		}

		operator rp3d::Vector2() const
		{
			rp3d::Vector2 vec;
			vec.x = x;
			vec.y = y;
			return vec;
		}

		// Operator=
		void operator=(glm::vec2 vec)
		{
			x = vec.x;
			y = vec.y;
		}

		void operator=(rp3d::Vector2 vec)
		{
			x = vec.x;
			y = vec.y;
		}

		// Operator+=
		void operator+=(glm::vec2 vec)
		{
			x += vec.x;
			y += vec.y;
		}

		void operator+=(rp3d::Vector2 vec)
		{
			x += vec.x;
			y += vec.y;
		}

		// Operator-=
		void operator-=(glm::vec2 vec)
		{
			x -= vec.x;
			y -= vec.y;
		}

		void operator-=(rp3d::Vector2 vec)
		{
			x -= vec.x;
			y -= vec.y;
		}

		// Operator+
		Vector2 operator+(Vector2 vec)
		{
			Vector2 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			return vector;
		}

		Vector2 operator+ (glm::vec2 vec)
		{
			Vector2 vector;
			vector = x + vec.x;
			vector = y + vec.y;
			return vector;
		}

		Vector2 operator+ (rp3d::Vector2 vec)
		{
			Vector2 vector;
			vector = x + vec.x;
			vector = y + vec.y;
			return vector;
		}

		// Operator-
		Vector2 operator-(Vector2 vec)
		{
			Vector2 vector;
			vector.x = x - vec.x;
			vector.y = y - vec.y;
			return vector;
		}

		Vector2 operator- (glm::vec2 vec)
		{
			Vector2 vector;
			vector = x - vec.x;
			vector = y - vec.y;
			return vector;
		}

		Vector2 operator- (rp3d::Vector2 vec)
		{
			Vector2 vector;
			vector = x - vec.x;
			vector = y - vec.y;
			return vector;
		}

		// Functions
		float Dot(Vector2 vec)
		{
			return (x * vec.x) + (y * vec.y);
		}

		/*Vector2 Cross(Vector2 vec)
		{
			Vector2 cross;
			cross.x = y * vec.z - z * vec.y;
			cross.y = x * vec.z - z * vec.x;
			cross.z = x * vec.y - y * vec.x;
			return cross;
		}*/

		float Length()
		{
			return sqrtf(x * x + y * y);
		}

		Vector2 Normalised()
		{
			Vector2 vector;
			float length = Length();

			vector.x / length;
			vector.y / length;

			return vector;
		}
	};

	struct Vector3
	{
		float x, y, z;

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

		Vector3(glm::vec3 vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		Vector3(rp3d::Vector3 vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		// Operator Overrides

		// Operator Conversion
		operator glm::vec3() const
		{
			glm::vec3 vec;
			vec.x = x;
			vec.y = y;
			vec.z = z;
			return vec;
		}

		operator rp3d::Vector3() const
		{
			rp3d::Vector3 vec;
			vec.x = x;
			vec.y = y;
			vec.z = z;
			return vec;
		}

		operator btVector3() const
		{
			btVector3 vec;
			vec.setValue(x, y, z);
			return vec;
		}

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

		void operator=(btVector3 vec)
		{
			x = vec.getX();
			y = vec.getY();
			z = vec.getZ();
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

		void operator+=(btVector3 vec)
		{
			x += vec.getX();
			y += vec.getY();
			z += vec.getZ();
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

		void operator-=(btVector3 vec)
		{
			x -= vec.getX();
			y -= vec.getY();
			z -= vec.getZ();
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
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			vector.z = z + vec.z;
			return vector;
		}

		Vector3 operator+ (rp3d::Vector3 vec)
		{
			Vector3 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			vector.z = z + vec.z;
			return vector;
		}

		Vector3 operator+(btVector3 vec)
		{
			Vector3 vector;
			vector.x = x + vec.getX();
			vector.y = y + vec.getY();
			vector.z = z + vec.getZ();
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

		Vector3 operator-(btVector3 vec)
		{
			Vector3 vector;
			vector.x = x - vec.getX();
			vector.y = y - vec.getY();
			vector.z = z - vec.getZ();
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
			vector.y / length;
			vector.z / length;

			return vector;
		}
	};

	struct Vector3Double
	{
		double x;
		double y;
		double z;
	};
}