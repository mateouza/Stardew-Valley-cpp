#include "soil.h"

#include <stdexcept>
#include <utility>

namespace game {

Soil::Soil(std::vector<std::vector<SoilState>> cells): cells(std::move(cells)) {
    if (this->cells.empty() || this->cells.front().empty()) {
        throw std::invalid_argument("El suelo debe tener al menos una celda");
    }

    const auto width = this->cells.front().size();
    for (const auto& row: this->cells) {
        if (row.size() != width) {
            throw std::invalid_argument("Las filas del suelo deben tener el mismo ancho");
        }
        for (const auto state: row) {
            if (state != SoilState::NotCultivable && state != SoilState::Unprepared &&
                state != SoilState::Prepared) {
                throw std::invalid_argument("El suelo contiene un estado de celda invalido");
            }
        }
    }
}

bool Soil::contains(std::size_t row, std::size_t column) const {
    return row < cells.size() && column < cells.front().size();
}

PrepareSoilResult Soil::prepare(std::size_t row, std::size_t column) {
    if (!contains(row, column)) {
        return PrepareSoilResult::OutOfBounds;
    }

    auto& state = cells[row][column];
    if (state == SoilState::NotCultivable) {
        return PrepareSoilResult::NotCultivable;
    }
    if (state == SoilState::Prepared) {
        return PrepareSoilResult::AlreadyPrepared;
    }

    state = SoilState::Prepared;
    return PrepareSoilResult::Accepted;
}

SoilState Soil::get_state(std::size_t row, std::size_t column) const {
    if (!contains(row, column)) {
        throw std::out_of_range("La celda consultada esta fuera del suelo");
    }
    return cells[row][column];
}

}
