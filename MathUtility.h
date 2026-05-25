#pragma once
#include "KamataEngine.h"

KamataEngine::Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotate, const KamataEngine::Vector3& translate);

inline KamataEngine::Vector3 operator+(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) { return KamataEngine::Vector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }

inline KamataEngine::Vector3 operator-(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) { return KamataEngine::Vector3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }

inline KamataEngine::Vector3 operator*(const KamataEngine::Vector3& v, float s) { return KamataEngine::Vector3(v.x * s, v.y * s, v.z * s); }