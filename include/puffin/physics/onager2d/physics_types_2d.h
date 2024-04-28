#pragma once

#include "puffin/types/vector.h"
#include "puffin/types/uuid.h"

#include <vector>

namespace puffin::physics
{
	struct AABB
	{
		Vector2f min;
		Vector2f max;
	};

	namespace collision2D
	{
		struct Contact
		{
			Contact()
			{
				a = 0;
				b = 0;

				pointOnA.zero();
				pointOnB.zero();
				normal.zero();

				seperation = 0.0f;
			}

			bool operator==(const Contact& other) const
			{
				return (a == other.a && b == other.b) || (a == other.b && b == other.a);
			}

			bool operator<(const Contact& other) const
			{
				return (a < other.a && b < other.b) || (a < other.b && b < other.a);
			}

			PuffinID a, b; // Entities which collided

			Vector2f pointOnA, pointOnB;
			Vector2f normal;
			float seperation;
		};

		class Simplex2D
		{
		public:

			Simplex2D() : mSize(0) {}

			Simplex2D& operator=(std::initializer_list<Vector2f> list)
			{
				for (auto v = list.begin(); v != list.end(); v++)
				{
					mPoints[std::distance(list.begin(), v)] = *v;
				}

				mSize = list.size();

				return *this;
			}

			void pushFront(Vector2f point)
			{
				mPoints = { point, mPoints[0], mPoints[1] };
				mSize = std::min(mSize + 1, 3u);
			}

			Vector2f operator[](uint32_t i) const { return mPoints[i]; }
			[[nodiscard]] uint32_t size() const { return mSize; }

			[[nodiscard]] auto begin() const { return mPoints.begin(); }
			[[nodiscard]] auto end() const { return mPoints.end() - (3 - mSize); }

		private:

			std::array<Vector2f, 3> mPoints;
			uint32_t mSize;

		};

		struct Edge
		{
			float distance = 0.0f;
			Vector2f normal = Vector2f(0.0f);
			int index = -1;
		};

		class Polygon2D
		{
		public:

			Polygon2D()
			{
				mPoints.reserve(32);
			};

			Polygon2D(const Simplex2D& simplex)
			{
				for (auto point : simplex)
				{
					mPoints.push_back(point);
				}
			}

			void insert(int index, Vector2f point)
			{
				mPoints.insert(mPoints.begin() + index, point);
			}

			Vector2f operator[](uint32_t i) { return mPoints[i]; }
			Vector2f operator[](uint32_t i) const { return mPoints[i]; }
			uint32_t size() const { return mPoints.size(); }

			[[nodiscard]] auto begin() const { return mPoints.begin(); }
			[[nodiscard]] auto end() const { return mPoints.end(); }

		private:

			std::vector<Vector2f> mPoints;
		};
	}
}