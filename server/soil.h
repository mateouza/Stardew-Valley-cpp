#ifndef SERVER_SOIL_H_
#define SERVER_SOIL_H_

#include <cstddef>
#include <vector>

#include "game_results.h"

namespace game {

enum class SoilState {
    NotCultivable,
    Unprepared,
    Prepared,
};


class Soil {
private:
    std::vector<std::vector<SoilState>> cells;

    bool contains(std::size_t row, std::size_t column) const;

public:
    explicit Soil(std::vector<std::vector<SoilState>> cells);

    std::size_t get_width() const;
    std::size_t get_height() const;

    PrepareSoilResult prepare(std::size_t row, std::size_t column);

    SoilState get_state(std::size_t row, std::size_t column) const;
};

}

#endif
