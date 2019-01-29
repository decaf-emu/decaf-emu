#include "latte_enum_as_string.h"

#undef LATTE_ENUM_CB_H
#undef LATTE_ENUM_COMMON_H
#undef LATTE_ENUM_DB_H
#undef LATTE_ENUM_PA_H
#undef LATTE_ENUM_PM4_H
#undef LATTE_ENUM_SPI_H
#undef LATTE_ENUM_SQ_H
#undef LATTE_ENUM_VGT_H

#include <common/enum_string_define.inl>
#include "latte_enum_cb.h"

#include <common/enum_string_define.inl>
#include "latte_enum_common.h"

#include <common/enum_string_define.inl>
#include "latte_enum_db.h"

#include <common/enum_string_define.inl>
#include "latte_enum_pa.h"

#include <common/enum_string_define.inl>
#include "latte_enum_pm4.h"

#include <common/enum_string_define.inl>
#include "latte_enum_spi.h"

// TODO: Fix collision in SQ_RES_OFFSET
// #include <common/enum_string_define.inl>
// #include "latte_enum_sq.h"

#include <common/enum_string_define.inl>
#include "latte_enum_vgt.h"
