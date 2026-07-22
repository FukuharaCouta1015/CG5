#include "WorldTransformEx.h"

using namespace KamataEngine;
using namespace KamataEngine::MathUtility;

void WorldTransformEx::UpdateMatrix() {
	// World変換行列を計算し、matWorld_に格納する
	matWorld_ = MakeAffineMatrix();
	// 定数バッファに転送する
	TransferMatrix();
}

Matrix4x4 WorldTransformEx::MakeAffineMatrix() {
	Matrix4x4 matScale = MakeScaleMatrix(scale_);

	Matrix4x4 matRotX = MakeRotateXMatrix(rotation_.x);
	Matrix4x4 matRotY = MakeRotateYMatrix(rotation_.y);
	Matrix4x4 matRotZ = MakeRotateZMatrix(rotation_.z);
	Matrix4x4 matRot = matRotZ * matRotX * matRotY;

	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);

	Matrix4x4 matWorld = matScale * matRot * matTrans;

	return matWorld;
}