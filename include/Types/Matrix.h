#pragma once

#ifndef MATRIX_H
#define MATRIX_H

#include <Types/Vector.h>



namespace puffin
{
	namespace Maths
	{
		/*
		====================
		Mat2
		====================
		*/
		
		struct Mat2
		{
			Mat2() {}

			Mat2(const Mat2& mat)
			{
				rows[0] = mat.rows[0];
				rows[1] = mat.rows[1];
			}

			Mat2(const float* mat)
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

			float Determinant() const
			{
				return rows[0].x * rows[1].y - rows[0].y * rows[1].x;
			}

			void Zero()
			{
				rows[0].Zero();
				rows[1].Zero();
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
			Mat3() {}

			Mat3(const Mat3& mat)
			{
				rows[0] = mat.rows[0];
				rows[1] = mat.rows[1];
				rows[2] = mat.rows[2];
			}

			Mat3(const float* mat)
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

			Vector2f operator* (const Vector2f& vec) const
			{
				Vector2f tmp;
				tmp[0] = rows[0].Dot(vec);
				tmp[1] = rows[1].Dot(vec);
				return tmp;
			}

			Vector3f operator* (const Vector3f& vec) const
			{
				Vector3f tmp;
				tmp[0] = rows[0].Dot(vec);
				tmp[1] = rows[1].Dot(vec);
				tmp[2] = rows[2].Dot(vec);
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
				rows[0].Zero();
				rows[1].Zero();
				rows[2].Zero();
			}

			void Identity()
			{
				rows[0] = Vector3(1.0f, 0.0f, 0.0f);
				rows[1] = Vector3(0.0f, 1.0f, 0.0f);
				rows[2] = Vector3(0.0f, 0.0f, 1.0f);
			}

			float Determinant() const
			{
				const float i = rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1]);
				const float j = rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0]);
				const float k = rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]);
				return (i - j + k);
			}

			Mat3 Transpose() const
			{
				Mat3 tranpose;
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 3; j++)
					{
						tranpose.rows[i][j] = rows[j][i];
					}
				}
				return tranpose;
			}

			Mat3 Inverse() const
			{
				Mat3 inv; 
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 3; j++)
					{
						inv.rows[j][i] = Cofactor(i, j);
					}
				}

				float det = Determinant();
				float invDet = 1.0f / det;
				inv *= invDet;
				return inv;
			}

			Mat2 Minor(const int i, const int j) const
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

			float Cofactor(const int i, const int j) const
			{
				const Mat2 minor = Minor(i, j);
				const float C = float(pow(-1, i + 1 + j + 1)) * minor.Determinant();
				return C;
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

#endif //MATRIX_H