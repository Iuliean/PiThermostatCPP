#ifndef I2C_BUS_HPP
#define I2C_BUS_HPP
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <expected>
#include <linux/i2c.h>
#include <type_traits>

#include "error.hpp"

namespace i2c
{
    enum class i2c_error_names
    {
        bus_error,
        transfer_error
    };

    using descriptor = int;
    using i2c_error = pi::error<i2c_error_names>;
    using block = std::array<std::byte, I2C_SMBUS_BLOCK_DATA>;
    using address = std::byte;

    class bus
    {
    public:
        static std::expected<bus, i2c_error> open(std::filesystem::path p) noexcept;

        explicit bus(descriptor fd) noexcept;
        ~bus() noexcept;

        bus(bus&& other) noexcept;
        bus& operator=(bus&& other) noexcept;

        bus(const bus&) = delete;
        bus& operator=(const bus&) = delete;
        
        std::expected<void, i2c_error> select_device(address address) const noexcept;

        std::expected<std::byte, i2c_error> read_byte(std::uint8_t command) const noexcept;
        std::expected<std::uint16_t, i2c_error> read_word(std::uint8_t command) const noexcept;
        std::expected<std::size_t, i2c_error> read_block(std::uint8_t command, block& data_block) const noexcept;

        std::expected<void, i2c_error> write(std::uint8_t command, std::uint8_t value) const noexcept;
        std::expected<void, i2c_error> write(std::uint8_t command, std::uint16_t value) const noexcept;
        std::expected<void, i2c_error> write(std::uint8_t command, std::span<const std::byte> data, bool partial = false) const noexcept;


    private:
        descriptor m_bus_descriptor;
    };

}

#endif //I2C_BUS_HPP