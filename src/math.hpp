#pragma once

#include "defines.hpp"

namespace storecast
{

struct vec3 {
  union {
    struct {
      f32 X;
      f32 Y;
      f32 Z;
    };
    f32 Data[3];
  };
};

} // namespace storecast
