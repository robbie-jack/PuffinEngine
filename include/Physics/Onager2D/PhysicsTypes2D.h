#pragma once

#include "Types/Vector.h"
#include "Types/UUID.h"

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

				pointOnA.Zero();
				pointOnB.Zero();
				normal.Zero();

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

			UUID a, b; // Entities which collided

			Vector2f pointOnA, pointOnB;
			Vector2f normal;
			float seperation;
		};

		class Simplex2D
		{
		public:

			Simplex2D() : m_size(0) {}

			Simplex2D& operator=(std::initializer_list<Vector2f> list)
			{
				for (auto v = list.begin(); v != list.end(); v++)
				{
					m_points[std::distance(list.begin(), v)] = *v;
				}

				m_size = list.size();

				return *this;
			}

			void pushFront(Vector2f point)
			{
				m_points = { point, m_points[0], m_points[1] };
				m_size = std::min(m_size + 1, 3u);
			}

			Vector2f operator[](uint32_t i) const { return m_points[i]; }
			[[nodiscard]] uint32_t size() const { return m_size; }

			[[nodiscard]] auto begin() const { return m_points.begin(); }
			[[nodiscard]] auto end() const { return m_points.end() - (3 - m_size); }

		private:

			std::array<Vector2f, 3> m_points;
			uint32_t m_size;

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
				points_.reserve(32);
			};

			Polygon2D(const Simplex2D& simplex)
			{
				for (auto point : simplex)
				{
					points_.push_back(point);
				}
			}

			void insert(int index, Vector2f point)
			{
				points_.insert(points_.begin() + index, point);
			}

			Vector2f operator[](uint32_t i) { return points_[i]; }
			Vector2f operator[](uint32_t i) const { return points_[i]; }
			uint32_t size() const { return points_.size(); }

			[[nodiscard]] auto begin() const { return points_.begin(); }
			[[nodiscard]] auto end() const { return points_.end(); }

		private:

			std::vector<Vector2f> points_;
		};
	}
}