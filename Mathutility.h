#pragma once
#include "KamataEngine.h"
#include <cmath>

// 拡大縮小・回転(X,Y,Z)・平行移動からワールド行列(アフィン変換行列)を作る自作関数。
// MathUtility.h 相当のものがこのプロジェクトには無いため、自前で用意しています。
// (行優先・行ベクトル前提。KamataEngineのMatrix4x4がfloat m[4][4]である想定です)
inline KamataEngine::Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotate, const KamataEngine::Vector3& translate) {
	using namespace KamataEngine;

	auto Multiply = [](const Matrix4x4& a, const Matrix4x4& b) {
		Matrix4x4 result{};
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++) {
				float sum = 0.0f;
				for (int k = 0; k < 4; k++) {
					sum += a.m[row][k] * b.m[k][col];
				}
				result.m[row][col] = sum;
			}
		}
		return result;
	};

	float cx = cosf(rotate.x), sx = sinf(rotate.x);
	float cy = cosf(rotate.y), sy = sinf(rotate.y);
	float cz = cosf(rotate.z), sz = sinf(rotate.z);

	Matrix4x4 scaleMat{};
	scaleMat.m[0][0] = scale.x;
	scaleMat.m[1][1] = scale.y;
	scaleMat.m[2][2] = scale.z;
	scaleMat.m[3][3] = 1.0f;

	Matrix4x4 rotateXMat{};
	rotateXMat.m[0][0] = 1.0f;
	rotateXMat.m[1][1] = cx;
	rotateXMat.m[1][2] = sx;
	rotateXMat.m[2][1] = -sx;
	rotateXMat.m[2][2] = cx;
	rotateXMat.m[3][3] = 1.0f;

	Matrix4x4 rotateYMat{};
	rotateYMat.m[0][0] = cy;
	rotateYMat.m[0][2] = -sy;
	rotateYMat.m[1][1] = 1.0f;
	rotateYMat.m[2][0] = sy;
	rotateYMat.m[2][2] = cy;
	rotateYMat.m[3][3] = 1.0f;

	Matrix4x4 rotateZMat{};
	rotateZMat.m[0][0] = cz;
	rotateZMat.m[0][1] = sz;
	rotateZMat.m[1][0] = -sz;
	rotateZMat.m[1][1] = cz;
	rotateZMat.m[2][2] = 1.0f;
	rotateZMat.m[3][3] = 1.0f;

	Matrix4x4 translateMat{};
	translateMat.m[0][0] = 1.0f;
	translateMat.m[1][1] = 1.0f;
	translateMat.m[2][2] = 1.0f;
	translateMat.m[3][0] = translate.x;
	translateMat.m[3][1] = translate.y;
	translateMat.m[3][2] = translate.z;
	translateMat.m[3][3] = 1.0f;

	Matrix4x4 result = Multiply(scaleMat, rotateXMat);
	result = Multiply(result, rotateYMat);
	result = Multiply(result, rotateZMat);
	result = Multiply(result, translateMat);
	return result;
}