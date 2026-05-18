#include "MapChipField.h"
#include <map>
#include <fstream>
#include <sstream>
#include "KamataEngine.h"

using namespace KamataEngine;

namespace {

    std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
};
}
    void MapChipField::ResetMapChipData() { 
       
        mapChipData_.data.clear(); 
        mapChipData_.data.resize(kNumBlockVertical);
	    for (std::vector<MapChipType>& mapChipDataline : mapChipData_.data) {
		    mapChipDataline.resize(kNumBlockHorizonal);
	    }
	   
    }

    void MapChipField::LoadMapChipCsv(const std::string& filePath) { 
		ResetMapChipData();

	    std::ifstream file;
	    file.open(filePath);
	    assert(file.is_open());

	    std::stringstream mapChipCsv;
	    mapChipCsv << file.rdbuf();
	    file.close();

	    for (uint32_t i = 0; i < kNumBlockVertical; ++i) {
		    std::string line;
		    getline(mapChipCsv, line);
		    std::stringstream lineStream(line);
		    for (uint32_t j = 0; j < kNumBlockHorizonal; ++j) {
			    std::string word;
			    std::getline(lineStream, word, ',');
			    if (mapChipTable.contains(word)) {
				    mapChipData_.data[i][j] = mapChipTable[word];
			    }
			}
		}
    }

	MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
		if (xIndex < 0 || kNumBlockHorizonal - 1 < xIndex) {
			return MapChipType::kBlank;
		}
		if (yIndex < 0 || kNumBlockVertical - 1 < yIndex) {
			return MapChipType::kBlank;
	    }
		return mapChipData_.data[yIndex][xIndex];
	  
    }
	Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
		return Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVertical - 1 - yIndex), 0); 
	}	