/*******************************************************************************
* Copyright 2023 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <cmath>

#include "dnnl_test_common.hpp"
#include "gtest/gtest.h"

#include "tests/test_isa_common.hpp"

#include "src/common/bit_cast.hpp"
#include "src/common/float8.hpp"

using dnnl::impl::utils::bit_cast;

namespace dnnl {

TEST(test_ref_float8_conversions, f8_e5m2_to_f32) {
    SKIP_IF(!impl::cpu::platform::has_data_type_support(
                    impl::data_type::f8_e5m2),
            "Engine does not support this data type.");
    // check all 256 f8_e5m2 values
    for (uint16_t u16 = 0; u16 <= 0xff; ++u16) {
        // convert f8_e5m2 to f32 and back again,
        // expecting bitwise idendical values except for sNaN,
        // where the convention is to set the quiet bit:
        // * +sNaN: 0x7d -> 0x7f
        // * -sNaN: 0xfd -> 0xff
        // f8_e5m2 encoding: seeeeemm
        //                         |_-> quiet bit is msb of mantissa
        uint8_t u8 = static_cast<uint8_t>(u16);
        constexpr bool is_bitcast = true;
        float8_e5m2_t x8(u8, is_bitcast);
        ASSERT_EQ(u8, x8.raw_bits_);
        ASSERT_EQ(u8, bit_cast<uint8_t>(x8));
        float x32(x8);
        float8_e5m2_t y8(x32);

        // if x8 is an sNaN the conversion sets the quiet bit (msb of mantissa)
        const bool is_x8_snan = impl::utils::one_of(u8, 0x7d, 0xfd);
        const uint8_t y8_expect = is_x8_snan ? u8 | 0x02 : u8;

        ASSERT_EQ(y8_expect, bit_cast<uint8_t>(y8));
    }
}

TEST(test_ref_float8_conversions, f8_e4m3_to_f32) {
    SKIP_IF(!impl::cpu::platform::has_data_type_support(
                    impl::data_type::f8_e4m3),
            "Engine does not support this data type.");
    // check all 256 f8_e4m3 values
    for (uint16_t u16 = 0; u16 <= 0xff; ++u16) {
        uint8_t u8 = static_cast<uint8_t>(u16);
        constexpr bool is_bitcast = true;
        float8_e4m3_t x8(u8, is_bitcast);
        ASSERT_EQ(u8, x8.raw_bits_);
        ASSERT_EQ(u8, bit_cast<uint8_t>(x8));

        // convert f8_e4m3 to f32 and back again,
        // expecting bitwise idendical values.
        // Note: f8_e4m3 does not have sNaN values, so no need to set quiet bit
        float x32(x8);
        float8_e4m3_t y8(x32);
        const uint8_t y8_expect = u8;
        ASSERT_EQ(y8_expect, bit_cast<uint8_t>(y8))
                << std::hex << std::endl
                << "u8 = " << static_cast<uint32_t>(u8) << std::endl
                << "x8.raw_bits_ = "
                << static_cast<uint32_t>(bit_cast<uint8_t>(x8)) << std::endl
                << "y8.raw_bits_ = "
                << static_cast<uint32_t>(bit_cast<uint8_t>(y8)) << std::endl
                << "y8_expect = " << static_cast<uint32_t>(y8_expect)
                << std::endl
                << std::dec;
    }
}

TEST(test_ref_float8_conversions, f32_to_f8_e4m3) {
    SKIP_IF(!impl::cpu::platform::has_data_type_support(
                    impl::data_type::f8_e4m3),
            "Engine does not support this data type.");
    // check all 2^32 f32 values
    impl::parallel_nd(0x100000000, [&](int64_t s64) {
        uint32_t u32 = static_cast<uint32_t>(s64);
        float x32 = bit_cast<float>(u32);
        ASSERT_EQ(u32, bit_cast<uint32_t>(x32));

        float16_t x16(x32);
        float8_e4m3_t x8_from_x16(x16);
        float8_e4m3_t x8_from_x32(x32);

        // check for double rounding
        ASSERT_EQ(x8_from_x16.raw_bits_, x8_from_x32.raw_bits_)
                << std::hex << std::endl
                << "x32 (raw bits) = " << bit_cast<uint32_t>(x32) << std::endl
                << "x16 (raw bits) = " << bit_cast<uint16_t>(x16) << std::endl
                << "x8_from_x16 (raw bits) = "
                << static_cast<uint32_t>(bit_cast<uint8_t>(x8_from_x16))
                << std::endl
                << "x8_from_x32 (raw_bits) = "
                << static_cast<uint32_t>(bit_cast<uint8_t>(x8_from_x32))
                << std::endl
                << std::dec;
    });
}

} // namespace dnnl
