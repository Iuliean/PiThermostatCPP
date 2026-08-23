#include "sensors/bmp280.hpp"

#include "error.hpp"
#include "i2c/i2c_bus.hpp"
#include "i2c/i2c_device.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <print>
#include <utility>

namespace pi
{

        constexpr static i2c::address g_bmp280_address{0x76};
        constexpr static std::uint8_t g_chip_id = 0x58;

        constexpr static std::uint8_t g_temp_xlsb_reg = 0xFC;
        constexpr static std::uint8_t g_temp_lsb_reg = 0xFB;
        constexpr static std::uint8_t g_temp_msb_reg = 0xFA;

        constexpr static std::uint8_t g_press_xlsb_reg = 0xF9;
        constexpr static std::uint8_t g_press_lsb_reg = 0xF8;
        constexpr static std::uint8_t g_press_msb_reg = 0xF7;


        constexpr static std::uint8_t g_config_reg = 0xF5;
        constexpr static std::uint8_t g_ctrl_meas_reg = 0xF4;
        constexpr static std::uint8_t g_status_reg = 0xF4;

        constexpr static std::uint8_t g_reset_reg = 0xE0;
        constexpr static std::uint8_t g_chip_id_reg = 0xD0;

        constexpr static std::uint8_t g_calibration_data_reg_start = 0x88;
        constexpr static std::size_t g_calibration_data_reg_end = 0xA1;


        std::expected<bmp280, bmp280_error> bmp280::create(const i2c::bus& bus)
        {
            const i2c::device bmp{bus, g_bmp280_address};

            return bmp.read_byte(g_chip_id_reg)
                .transform_error([](i2c::i2c_error err){
                    return bmp280_error{
                        bmp280_error_names::protocol_error,
                        std::move(err).message()
                    };
                })
                .and_then([](std::byte id_byte) -> std::expected<void, bmp280_error> {
                    const auto id = std::to_integer<std::uint8_t>(id_byte); 
                    if (id != g_chip_id)
                        return std::unexpected{
                            bmp280_error{
                                bmp280_error_names::chip_error,
                                std::format("Invalid chip id: {}", id)
                            }
                        };
                    return {};
                })
                .and_then([&bmp, &bus]() -> std::expected<bmp280, bmp280_error> {
                    bmp280 sensor{bmp};

                    RETURN_VALUE_IF_EXPECTED_ELSE_FORWARD(
                        sensor.fetch_calibration_data(),
                        sensor
                    );
                });
        }

        bmp280::bmp280(i2c::device device)
            : m_device(device) { }

        std::expected<void, bmp280_error> bmp280::reset()const noexcept
        {
            return m_device.write(g_reset_reg, static_cast<std::uint8_t>(0xB6))
                .transform_error([](i2c::i2c_error err){
                    return bmp280_error{
                            bmp280_error_names::protocol_error,
                            std::move(err).message()
                        };
                    });
        }

        std::expected<std::uint8_t, bmp280_error> bmp280::chip_id()const noexcept
        {
            return m_device.read_byte(g_chip_id_reg)
                .transform([](std::byte b) { return std::to_integer<std::uint8_t>(b);})
                .transform_error([](i2c::i2c_error err){
                    return bmp280_error{
                        bmp280_error_names::protocol_error,
                        std::move(err).message()
                    };
                });
        }

        std::expected<std::int32_t, bmp280_error> bmp280::get_temp() const noexcept
        {
            std::array<std::byte, 3> temp_data{std::byte{0}};

            return m_device.read_block(g_temp_msb_reg, temp_data)
                .transform_error([](i2c::i2c_error err){
                    return bmp280_error{
                        bmp280_error_names::protocol_error,
                        std::move(err).message()
                    };
                })
                .and_then([this, &temp_data](std::size_t bytes_read) -> std::expected<std::int32_t, bmp280_error> {
                    //the values are extended on 3 registers
                    if (bytes_read != 3)
                        return std::unexpected{
                            bmp280_error{
                                bmp280_error_names::protocol_error,
                                "Block read more or less bytes"
                            }
                        };

                    return compensate_temperature(
                        (std::to_integer<std::int32_t>(temp_data[0])  << 12) |
                        (std::to_integer<std::int32_t>(temp_data[1])  << 4) |
                        (std::to_integer<std::int32_t>(temp_data[2])  >> 4)
                    );
                });
        }

        std::expected<std::uint32_t, bmp280_error> bmp280::get_pressure() const noexcept
        {
            std::array<std::byte, 6> temp_press_data;

            return m_device.read_block(g_press_msb_reg, temp_press_data)
                .transform_error([](i2c::i2c_error err){
                    return bmp280_error{
                        bmp280_error_names::protocol_error,
                        std::move(err).message()
                    };
                })
                .and_then([this, &temp_press_data](std::size_t bytes_read) -> std::expected<std::uint32_t, bmp280_error> {
                    if(bytes_read != 6)
                        return std::unexpected{
                            bmp280_error{
                                bmp280_error_names::protocol_error,
                                "Block read more or less bytes than requested"
                            }
                        };

                    return compensate_pressure(
                        //press data
                        (std::to_integer<std::int32_t>(temp_press_data[0]) << 12) |
                        (std::to_integer<std::int32_t>(temp_press_data[1]) << 4) |
                        (std::to_integer<std::int32_t>(temp_press_data[2]) >> 4)
                        ,//Temp data
                        (std::to_integer<std::int32_t>(temp_press_data[3]) << 12) |
                        (std::to_integer<std::int32_t>(temp_press_data[4]) << 4) |
                        (std::to_integer<std::int32_t>(temp_press_data[5]) >> 4)
                    );
                });

            return 0;
        }

        std::expected<void, bmp280_error> bmp280::set_config(config conf) const noexcept
        {
            return m_device.write(g_ctrl_meas_reg, conf.value())
                .transform_error([](i2c::i2c_error err){
                    return bmp280_error{
                        bmp280_error_names::protocol_error,
                        std::move(err).message()
                    };
                });
        }

        /**********
        * PRIVATE *
        **********/

        std::expected<void, bmp280_error> bmp280::fetch_calibration_data()
        {
            std::span<std::byte> data_block{
                reinterpret_cast<std::byte*>(&m_calib_data),
                reinterpret_cast<std::byte*>(&m_calib_data) + (g_calibration_data_reg_end - g_calibration_data_reg_start)
            };

            RETURN_VALUE_IF_EXPECTED_ELSE_FORWARD(
                m_device.read_block(g_calibration_data_reg_start, data_block)
                    .transform_error([](i2c::i2c_error err){
                        return bmp280_error{
                            bmp280_error_names::protocol_error,
                            std::move(err).message()
                        };
                    }),
                {}
            );
        }

        /* Implementation from https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf*/
        constexpr std::int32_t bmp280::get_fine_temp(std::int32_t raw_temp) const noexcept
        {
            const std::int32_t temp1 = (((raw_temp >> 3) - ((static_cast<std::int32_t>(m_calib_data.dig_T1) << 1))) * static_cast<std::int32_t>(m_calib_data.dig_T2)) >> 11;
            const std::int32_t temp2 =  ((((raw_temp >> 4) - static_cast<std::int32_t>(m_calib_data.dig_T1)) * ((raw_temp >> 4) - static_cast<std::int32_t>(m_calib_data.dig_T1))) >> 12) * static_cast<std::int32_t>(m_calib_data.dig_T3) >> 14;
            return temp1 + temp2;
        }

        /* Implementation from https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf*/
        constexpr std::int32_t bmp280::compensate_temperature(std::int32_t raw_temp) const noexcept
        {
            return ((get_fine_temp(raw_temp)) * 5 + 128) >> 8;
        }


        /* Implementation from https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf*
            WARNING I DID NOT BOTHER TO TEST IF IT DOES THE ACTUAL THING I COULD NOT VERIFY IT AND I WAS LAZY
        */
        constexpr std::uint32_t bmp280::compensate_pressure(std::int32_t raw_pressure, std::int32_t raw_temp) const noexcept
        {
            std::int64_t temp1;
            std::int64_t temp2;
            std::int64_t pressure;

            temp1 = static_cast<std::int64_t>(get_fine_temp(raw_temp));
            temp2 = temp1 * temp1 * static_cast<std::int64_t>(m_calib_data.dig_P6);
            temp2 = temp2 + ((temp1 * static_cast<std::int64_t>(m_calib_data.dig_P5)) << 17);
            temp2 = temp2 + (static_cast<std::int64_t>(m_calib_data.dig_P4) << 35);
            temp1 = ((temp1 * temp1 *m_calib_data.dig_P3) >> 8) + ((temp1 * static_cast<std::int64_t>(m_calib_data.dig_P2)) << 12);
            temp1 = ((static_cast<std::int64_t>(1) << 47) + temp1) * static_cast<std::int64_t>(m_calib_data.dig_P1) >> 33;

            if (temp1 == 0) return 0;
            
            pressure = 1048576 - raw_pressure;
            pressure = (((pressure << 31) - temp2) * 3125) / temp1;

            temp1 = static_cast<std::int64_t>(m_calib_data.dig_P9) * (pressure >> 13) * (pressure >> 13 ) >> 25;
            temp2 = (static_cast<std::int64_t>(m_calib_data.dig_P8) * pressure) >> 19;

            pressure = ((pressure + temp1 + temp2) >> 8) + (static_cast<std::int64_t>(m_calib_data.dig_P7) << 4);
            return static_cast<std::uint32_t>(pressure);
        }
}