#pragma once

#include "types/vector2.h"
#include "types/vector3.h"
#include "glm/matrix.hpp"

namespace puffin
{
	namespace maths
	{
		/*
		====================
		Mat2
		====================
		*/
		
		struct Mat2
		{
			Mat2() = default;

			Mat2(const Mat2& mat)
			{
				rows[0] = mat.rows[0];
				rows[1] = mat.rows[1];
			}

			explicit Mat2(const float* mat)
			{
				rows[0] = mat + 0;
				rows[1] = mat + 2;
			}

			Mat2(const Vector2f& row0, const Vector2f& row1)
			{
				rows[0] = row0;
				rows[1] = row1;
			}

			Mat2& operator=(const Mat2& mat)
			{
				rows[0] = mat.rows[0];
				rows[1] = mat.rows[1];
				return *this;
			}

			const Mat2& operator*=(const float val)
			{
				rows[0] *= val;
				rows[1] *= val;
				return *this;
			}

			const Mat2& operator+=(const Mat2& mat)
			{
				rows[0] += mat.rows[0];
				rows[1] += mat.rows[1];
				return *this;
			}

			[[nodiscard]] float Determinant() const
			{
				return rows[0].x * rows[1].y - rows[0].y * rows[1].x;
			}

			void Zero()
			{
				rows[0].zero();
				rows[1].zero();
			}

			Vector2f rows[2];
		};
//
		/*
		====================
		Mat3
		====================
		*/

		struct Mat3
		{
			Mat3() = default;

			Mat3(const Mat3& mat)
			{
				rows[0] = mat.rows[0];
				rows[1] = mat.rows[1];
				rows[2] = mat.rows[2];
			}

			explicit Mat3(const float* mat)
			{
				rows[0] = mat + 0;
				rows[1] = mat + 3;
				rows[2] = mat + 6;
			}

			Mat3(const Vector3f& row0, const Vector3f& row1, const Vector3f& row2)
			{
				rows[0] = row0;
				rows[1] = row1;
				rows[2] = row2;
			}

			Mat3& operator=(const Mat3& mat)
			{
				rows[0] = mat.rows[0];
				rows[1] = mat.rows[1];
				rows[2] = mat.rows[2];
				return *this;
			}

			explicit operator glm::mat3() const
			{
				glm::mat3 mat;

				mat[0] = static_cast<glm::vec3>(rows[0]);
				mat[1] = static_cast<glm::vec3>(rows[1]);
				mat[2] = static_cast<glm::vec3>(rows[2]);

				return mat;
			}

			Vector2f operator* (const Vector2f& vec) const
			{
				Vector2f tmp;
				tmp[0] = rows[0].dot(vec);
				tmp[1] = rows[1].dot(vec);
				return tmp;
			}

			Vector3f operator* (const Vector3f& vec) const
			{
				Vector3f tmp;
				tmp[0] = rows[0].dot(vec);
				tmp[1] = rows[1].dot(vec);
				tmp[2] = rows[2].dot(vec);
				return tmp;
			}

			Mat3 operator* (const float val) const
			{
				Mat3 tmp;
				tmp.rows[0] = rows[0] * val;
				tmp.rows[1] = rows[1] * val;
				tmp.rows[2] = rows[2] * val;
				return tmp;
			}

			Mat3 operator* (const Mat3& mat) const
			{
				Mat3 tmp;
				for (int i = 0; i < 3; i++)
				{
					tmp.rows[i].x = rows[i].x * mat.rows[0].x + rows[i].y * mat.rows[1].x + rows[i].z * mat.rows[2].x;
					tmp.rows[i].y = rows[i].x * mat.rows[0].y + rows[i].y * mat.rows[1].y + rows[i].z * mat.rows[2].y;
					tmp.rows[i].z = rows[i].x * mat.rows[0].z + rows[i].y * mat.rows[1].z + rows[i].z * mat.rows[2].z;
				}
				return tmp;
			}

			Mat3 operator+ (const Mat3& mat) const
			{
				Mat3 tmp;
				for (int i = 0; i < 3; i++)
				{
					tmp.rows[i] = rows[i] + mat.rows[i];
				}
				return tmp;
			}

			const Mat3& operator*=(const float val)
			{
				rows[0] *= val;
				rows[1] *= val;
				rows[2] *= val;
				return *this;
			}

			const Mat3& operator+=(const Mat3& mat)
			{
				rows[0] += mat.rows[0];
				rows[1] += mat.rows[1];
				rows[2] += mat.rows[2];
				return *this;
			}

			void Zero()
			{
				rows[0].zero();
				rows[1].zero();
				rows[2].zero();
			}

			void Identity()
			{
				rows[0] = Vector3(1.0f, 0.0f, 0.0f);
				rows[1] = Vector3(0.0f, 1.0f, 0.0f);
				rows[2] = Vector3(0.0f, 0.0f, 1.0f);
			}

			[[nodiscard]] float Determinant() const
			{
				const float i = rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1]);
				const float j = rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0]);
				const float k = rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]);
				return (i - j + k);
			}

			[[nodiscard]] Mat3 Transpose() const
			{
				Mat3 transpose;
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 3; j++)
					{
						transpose.rows[i][j] = rows[j][i];
					}
				}
				return transpose;
			}

			[[nodiscard]] Mat3 Inverse() const
			{
				Mat3 inv; 
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 3; j++)
					{
						inv.rows[j][i] = Cofactor(i, j);
					}
				}

				const float det = Determinant();
				const float invDet = 1.0f / det;
				inv *= invDet;
				return inv;
			}

			[[nodiscard]] Mat2 Minor(const int i, const int j) const
			{
				Mat2 minor;

				int yy = 0;
				for (int y = 0; y < 3; y++)
				{
					if (y == j)
						continue;

					int xx = 0;
					for (int x = 0; x < 3; x++)
					{
						if (x == i)
							continue;

						minor.rows[xx][yy] = rows[x][y];

						xx++;
					}

					yy++;
				}

				return minor;
			}

			[[nodiscard]] float Cofactor(const int i, const int j) const
			{
				const Mat2 minorMat = Minor(i, j);
				const float cofactor = static_cast<float>(pow(-1, i + 1 + j + 1)) * minorMat.Determinant();
				return cofactor;
			}

			Vector3f rows[3];
		};

		/*
		====================
		Mat4
		====================
		*/


	}
}