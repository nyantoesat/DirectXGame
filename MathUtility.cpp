#include "MathUtility.h"
#include <cmath>

using namespace KamataEngine;

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {

	Matrix4x4 matrix{};

	float cosX = cosf(rotate.x);
	float sinX = sinf(rotate.x);

	float cosY = cosf(rotate.y);
	float sinY = sinf(rotate.y);

	float cosZ = cosf(rotate.z);
	float sinZ = sinf(rotate.z);

	matrix.m[0][0] = scale.x * (cosY * cosZ);
	matrix.m[0][1] = scale.x * (cosY * sinZ);
	matrix.m[0][2] = scale.x * (-sinY);
	matrix.m[0][3] = 0.0f;

	matrix.m[1][0] = scale.y * (sinX * sinY * cosZ - cosX * sinZ);
	matrix.m[1][1] = scale.y * (sinX * sinY * sinZ + cosX * cosZ);
	matrix.m[1][2] = scale.y * (sinX * cosY);
	matrix.m[1][3] = 0.0f;

	matrix.m[2][0] = scale.z * (cosX * sinY * cosZ + sinX * sinZ);
	matrix.m[2][1] = scale.z * (cosX * sinY * sinZ - sinX * cosZ);
	matrix.m[2][2] = scale.z * (cosX * cosY);
	matrix.m[2][3] = 0.0f;

	matrix.m[3][0] = translate.x;
	matrix.m[3][1] = translate.y;
	matrix.m[3][2] = translate.z;
	matrix.m[3][3] = 1.0f;

	return matrix;
}