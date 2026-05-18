#include "Skydome.h"
#include "MathUtility.h"

using namespace KamataEngine;	

void Skydome::Initialize() { 
	skytextureHandle_ = TextureManager::Load("./Resources/SkyDome/sky_sphere.png");
	
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	
	worldTransform_.Initialize();

	worldTransform_.scale_ = {-100, 100, 100};
}

void Skydome::Update() { 
	worldTransform_.TransferMatrix();
	 }

void Skydome::Draw(Camera& camera) { 

	modelSkydome_->Draw(worldTransform_, camera, skytextureHandle_);
}	