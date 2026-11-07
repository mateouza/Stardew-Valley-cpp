#ifndef COMMON_CROP_STATE_H_
#define COMMON_CROP_STATE_H_

#include <cstdint>

namespace game {

enum class CropType : std::uint8_t {
    Strawberry = 0,
};

struct CropState {
    CropType type = CropType::Strawberry;
    bool needs_water = true;
};

}

#endif
