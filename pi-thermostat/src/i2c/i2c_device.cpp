#include "i2c/i2c_device.hpp"
#include <memory>

namespace i2c
{

    device::device(const bus& bus, address address) noexcept
        : m_i2c_bus(std::addressof(bus)),
          m_address(address)
    { }

    std::expected<std::byte, i2c_error> device::read_byte(std::uint8_t command) const noexcept
        {
            return select_this_device()
                .and_then([this, command](){
                    return m_i2c_bus->read_byte(command);
                }
            );
        }

        std::expected<std::uint16_t, i2c_error> device::read_word(std::uint8_t command) const noexcept
        {
            return select_this_device()
                .and_then([this, command](){
                    return m_i2c_bus->read_word(command);
                }
            );
        }

        std::expected<std::size_t, i2c_error> device::read_block(std::uint8_t command, std::span<std::byte> data_block) const noexcept
        {
            return select_this_device()
                .and_then([this, command, data_block](){
                    return m_i2c_bus->read_block(command, data_block);
                }
            );
        }

        std::expected<void, i2c_error> device::write(std::uint8_t command, std::uint8_t value) const noexcept
        {
            return select_this_device()
                .and_then([this, command, value](){
                    return m_i2c_bus->write(command, value);
                }
            );
        }

        std::expected<void, i2c_error> device::write(std::uint8_t command, std::uint16_t value) const noexcept
        {
            return select_this_device()
                .and_then([this, command, value](){
                    return m_i2c_bus->write(command, value);
                }
            );
        }

        std::expected<void, i2c_error> device::write(std::uint8_t command, std::span<const std::byte> data, bool partial) const noexcept
        {
            return select_this_device()
                .and_then([this, command, data, partial](){
                    return m_i2c_bus->write(command, data, partial);
                }
            );
        }



}