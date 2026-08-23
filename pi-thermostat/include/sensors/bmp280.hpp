#ifndef BMP280_HPP
#define BMP280_HPP
#include <cstdint>
#include <expected>
#include <utility>

#include "i2c/i2c_bus.hpp"
#include "i2c/i2c_device.hpp"

namespace pi
{

    enum class bmp280_error_names
    {
        protocol_error,
        chip_error
    };

    using bmp280_error = pi::error<bmp280_error_names>;

    class bmp280
    {
    public:

        enum class temperature_oversampling : std::uint8_t
        {
            disable = 0,
            x1      = 0b00100000,
            x2      = 0b01000000,
            x4      = 0b01100000,
            x8      = 0b10000000,
            x16     = 0b11100000,
        };

        enum class pressure_oversampling : std::uint8_t
        {
            disable = 0,
            x1      = 0b00000100,
            x2      = 0b00001000,
            x4      = 0b00001100,
            x8      = 0b00010011,
            x16     = 0b00011100,
        };

        enum class power_mode : std::uint8_t
        {
            sleep  = 0,
            normal = 0b00000011,
            forced = 0b00000010
        };

        enum class standby_time : std::uint8_t
        {
            ms_0_5  = 0,
            ms_62_5 = 0b00100000,
            ms_125  = 0b01000000,
            ms_250  = 0b01100000,
            ms_500  = 0b10000000,
            ms_1000 = 0b10100000,
            ms_2000 = 0b11000000,
            ms_4000 = 0b11100000,
        };

        enum class iir_filter_coef: std::uint8_t
        {
            off = 0b00000000,
            x2 = 0b00001000,
            x4= 0b00001000,
            x8 = 0b00001100,
            x16 = 0b00011100,
        };

        enum class spi_3wires
        {
            enable = 0b000000001,
            disable = 0,
        };

        struct calibration_data
        {
            std::uint16_t dig_T1;
            std::int16_t dig_T2;
            std::int16_t dig_T3;
            std::uint16_t dig_P1;
            std::int16_t dig_P2;
            std::int16_t dig_P3;
            std::int16_t dig_P4;
            std::int16_t dig_P5;
            std::int16_t dig_P6;
            std::int16_t dig_P7;
            std::int16_t dig_P8;
            std::int16_t dig_P9;
            std::int16_t reserved;
        };

        class config
        {
        public:
            constexpr config() noexcept
                : m_config(0), m_ctrl_meas(0) {}
            constexpr ~config() noexcept = default;

            constexpr config& set_temperature_oversampling(temperature_oversampling sampling) noexcept
            { m_ctrl_meas |= std::to_underlying(sampling); return *this; }

            constexpr config& set_pressure_oversampling(pressure_oversampling sampling) noexcept
            { m_ctrl_meas |= std::to_underlying(sampling); return *this; }

            constexpr config& set_power_mode(power_mode mode) noexcept
            { m_ctrl_meas |= std::to_underlying(mode); return *this; }

            constexpr config& set_standby_time(standby_time time) noexcept
            { m_config |= std::to_underlying(time); return *this; }

            constexpr config& set_iir_filter_coef(iir_filter_coef coef) noexcept
            { m_config |= std::to_underlying(coef); return *this; }

            constexpr config& set_spi_3wires(spi_3wires spi) noexcept
            { m_config |= std::to_underlying(spi); return *this; }

            constexpr std::uint16_t value() const noexcept
            {
                std::uint16_t out = 0;
                out = m_config;
                out <<= 8;
                out = m_ctrl_meas;
                return out;
            }

        private:
            std::uint8_t m_config;
            std::uint8_t m_ctrl_meas;
        };

        static std::expected<bmp280, bmp280_error> create(const i2c::bus& bus);

        bmp280(i2c::device device);
        ~bmp280() = default;

        std::expected<void, bmp280_error> reset()const noexcept;

        std::expected<std::uint8_t, bmp280_error> chip_id() const noexcept;
        std::expected<std::int32_t, bmp280_error> get_temp() const noexcept;
        std::expected<std::uint32_t, bmp280_error> get_pressure() const noexcept;
        std::expected<void, bmp280_error> set_config(config conf) const noexcept;

    private:

        std::expected<void, bmp280_error> fetch_calibration_data();
        constexpr std::int32_t get_fine_temp(std::int32_t raw_temp) const noexcept;
        constexpr std::int32_t compensate_temperature(std::int32_t raw_temp) const noexcept;
        constexpr std::uint32_t compensate_pressure(std::int32_t raw_pressure, std::int32_t raw_temp) const noexcept;
        
        i2c::device m_device;
        calibration_data m_calib_data;
        
    };
}

#endif //BMP280_HPP