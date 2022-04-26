#pragma once

#include "ECS/ECS.h"

namespace Puffin::Physics
{
	struct AABB
	{
		Vector2f min;
		Vector2f max;
	};

	namespace Collision2D
	{
		struct Contact
		{
			Contact() {}

			ECS::Entity a, b; // Entities which collided

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

			void push_front(Vector2f point)
			{
				m_points = { point, m_points[0], m_points[1] };
				m_size = std::min(m_size + 1, 3u);
			}

			Vector2f operator[](uint32_t i) { return m_points[i]; }
			uint32_t size() const { return m_size; }

			auto begin() const { return m_points.begin(); }
			auto end() const { return m_points.end() - (3 - m_size); }

		private:

			std::array<Vector2f, 3> m_points;
			uint32_t m_size;

		};

		struct Edge
		{
			float distance;
			Vector2f normal;
			int index;
		};

		class Polygon2D
		{
		public:

			Polygon2D() {};

			Polygon2D(const Simplex2D& simplex)
			{
				for (auto point : simplex)
				{
					m_points.push_back(point);
				}
			}

			void insert(int index, Vector2f point)
			{
				m_points.insert(m_points.begin() + index, point);
			}

			Vector2f operator[](uint32_t i) { return m_points[i]; }
			const Vector2f operator[](uint32_t i) const { return m_points[i]; }
			uint32_t size() const { return m_points.size(); }

			auto begin() const { return m_points.begin(); }
			auto end() const { return m_points.end(); }

		private:

			std::vector<Vector2f> m_points;
		};
	}
}