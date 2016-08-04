#pragma once
#include <sg14/fixed_point.h>

typedef sg14::make_fixed<8, 8, int16_t> fixed88_t;
typedef sg14::make_ufixed<8, 8, uint16_t> ufixed88_t;

typedef sg14::make_fixed<16, 16, int32_t> fixed1616_t;
typedef sg14::make_ufixed<16, 16, uint32_t> ufixed1616_t;
typedef sg14::make_fixed<0, 16, int32_t> fixed016_t;
typedef sg14::make_ufixed<0, 16, uint32_t> ufixed016_t;
typedef sg14::make_fixed<16, 0, int32_t> fixed160_t;
typedef sg14::make_ufixed<16, 0, uint32_t> ufixed160_t;
