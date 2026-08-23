#ifndef I2C_DEVICE_HPP
#define I2C_DEVICE_HPP
#include "i2c/i2c_bus.hpp"

namespace i2c
{
    class device
    {
    public:
        device(const bus& bus, address address) noexcept;
        ~device() = default;


        std::expected<std::byte, i2c_error> read_byte(std::uint8_t command) const noexcept;
        std::expected<std::uint16_t, i2c_error> read_word(std::uint8_t command) const noexcept;
        std::expected<std::size_t, i2c_error> read_block(std::uint8_t command, std::span<std::byte> data_block) const noexcept;

        std::expected<void, i2c_error> write(std::uint8_t command, std::uint8_t value) const noexcept;
        std::expected<void, i2c_error> write(std::uint8_t command, std::uint16_t value) const noexcept;
        std::expected<void, i2c_error> write(std::uint8_t command, std::span<const std::byte> data, bool partial = false) const noexcept;
    private:

    std::expected<void, i2c_error> select_this_device() const noexcept { return m_i2c_bus->select_device(m_address); }

        const bus* m_i2c_bus;
        address m_address;
    };
}


#endif //I2C_DEVICE_HPP