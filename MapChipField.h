#pragma once
#include "KamataEngine.h"
using namespace KamataEngine;
enum class MapChipType {
	kBlank,
	kBlock,
};
struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField {
public:
	void ResetMapChipData();
	void LoadMapChipCsv(const std::string& filePath);
	MapChipData mapChipData_;
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);
	uint32_t GetNumBlockVertical() const { return kNumBlockVertical; }
	uint32_t GetNumBlockHorizontal() const { return kNumBlockHorizonal; }

private:
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;
	static inline const uint32_t kNumBlockVertical = 20;
	static inline const uint32_t kNumBlockHorizonal = 100;
};