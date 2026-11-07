#include "soil.h"

#include <cstdint>
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

std::size_t Soil::get_width() const { return cells.front().size(); }

std::size_t Soil::get_height() const { return cells.size(); }

SoilData Soil::get_data() const {
    const auto width = get_width();
    const auto height = get_height();
    if (!std::in_range<std::uint32_t>(width) || !std::in_range<std::uint32_t>(height)) {
        throw std::overflow_error("Las dimensiones del suelo no entran en SoilData");
    }

    SoilData data;
    data.width = static_cast<std::uint32_t>(width);
    data.height = static_cast<std::uint32_t>(height);
    if (height > data.cells.max_size() / width) {
        throw std::length_error("El suelo supera la capacidad del vector de celdas");
    }

    data.cells.reserve(width * height);
    for (const auto& row: cells) {
        data.cells.insert(data.cells.end(), row.begin(), row.end());
    }
    return data;
}

bool Soil::contains(std::size_t row, std::size_t column) const {
    return row < get_height() && column < get_width();
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


PlantCropResult Soil::plant(std::size_t row, std::size_t column, CropType type) {
    if (!contains(row, column)) {
        return PlantCropResult::OutOfBounds;
    }
    if (type != CropType::Strawberry) {
        return PlantCropResult::InvalidCrop;
    }
    if (cells[row][column] == SoilState::NotCultivable) {
        return PlantCropResult::NotCultivable;
    }
    if (cells[row][column] != SoilState::Prepared) {
        return PlantCropResult::SoilNotPrepared;
    }
    if (!crops.emplace(std::make_pair(row, column), CropState{type}).second) {
        return PlantCropResult::AlreadyPlanted;
    }
    return PlantCropResult::Accepted;
}

WaterCropResult Soil::water(std::size_t row, std::size_t column) {
    if (!contains(row, column)) {
        return WaterCropResult::OutOfBounds;
    }
    const auto entry = crops.find({row, column});
    if (entry == crops.end()) {
        return WaterCropResult::NoCrop;
    }
    if (!entry->second.needs_water) {
        return WaterCropResult::AlreadyWatered;
    }
    entry->second.needs_water = false;
    return WaterCropResult::Accepted;
}

std::optional<CropState> Soil::get_crop_state(std::size_t row, std::size_t column) const {
    if (!contains(row, column)) {
        throw std::out_of_range("La celda consultada esta fuera del suelo");
    }
    const auto entry = crops.find({row, column});
    if (entry == crops.end()) {
        return std::nullopt;
    }
    return entry->second;
}


SoilState Soil::get_state(std::size_t row, std::size_t column) const {
    if (!contains(row, column)) {
        throw std::out_of_range("La celda consultada esta fuera del suelo");
    }
    return cells[row][column];
}

}
