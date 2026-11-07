#ifndef SERVER_SOIL_H_
#define SERVER_SOIL_H_

#include <cstddef>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "crop_state.h"
#include "game_results.h"
#include "soil_data.h"

namespace game {

class Soil {
private:
    std::vector<std::vector<SoilState>> cells;
    std::map<std::pair<std::size_t, std::size_t>, CropState> crops;

    bool contains(std::size_t row, std::size_t column) const;

public:
    explicit Soil(std::vector<std::vector<SoilState>> cells);

    std::size_t get_width() const;
    std::size_t get_height() const;

    PrepareSoilResult prepare(std::size_t row, std::size_t column);
    PlantCropResult plant(std::size_t row, std::size_t column, CropType type);
    WaterCropResult water(std::size_t row, std::size_t column);

    SoilState get_state(std::size_t row, std::size_t column) const;

    std::optional<CropState> get_crop_state(std::size_t row, std::size_t column) const;

    SoilData get_data() const;
};

}

#endif
