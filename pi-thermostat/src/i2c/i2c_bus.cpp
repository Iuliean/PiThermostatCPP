#include <cstddef>
#include <cstring>
#include <expected>

extern "C"
{
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include <sys/ioctl.h>

#include "i2c/i2c_bus.hpp"

namespace i2c
{

    template<typename T>
    static std::expected<T, i2c_error> perform_i2c_read(descriptor fd, std::uint8_t command, auto fn) noexcept
    {
        const auto result = fn(fd, command);

        if (result < 0)
            return std::expected<T, i2c_error>{
                std::unexpect,
                i2c_error_names::transfer_error,
                ::strerror(-result)
            };

        return std::expected<T, i2c_error>{
            std::in_place,
            static_cast<T>(result)
        };

    }

    static std::expected<void, i2c_error> perform_i2c_write(descriptor fd, std::uint8_t command, auto value, auto fn) noexcept
    {
        const auto result = fn(fd, command, value);

        if (result < 0)
            return std::expected<void, i2c_error>{
                std::unexpect,
                i2c_error_names::transfer_error,
                ::strerror(-result)
            };

        return {};
    }

    std::expected<bus, i2c_error> bus::open(std::filesystem::path path) noexcept
    {
        const descriptor fd = ::open(std::filesystem::absolute(path).c_str(), O_RDWR);

        if ( fd < 0)
            return std::expected<bus, i2c_error>{
                std::unexpect,
                i2c_error_names::bus_error,
                ::strerror(errno)
            };

        return std::expected<bus, i2c_error>{
            std::in_place,
            fd
        };
    }

    bus::bus(descriptor fd) noexcept
        : m_bus_descriptor(fd) { }

    bus::~bus()noexcept
    {
        ::close(m_bus_descriptor);
    }

    bus::bus(bus&& other) noexcept
        : m_bus_descriptor( other.m_bus_descriptor)
    {
        other.m_bus_descriptor = -1;
    }

    bus& bus::operator=(bus&& other) noexcept
    {
        m_bus_descriptor = other.m_bus_descriptor;
        other.m_bus_descriptor = -1;
        return *this;
    }

    std::expected<void, i2c_error> bus::select_device(address address) const noexcept
    {
        const auto result = ::ioctl(m_bus_descriptor, I2C_SLAVE, address);

        if (result < 0)
            return std::expected<void, i2c_error>{
                std::unexpect,
                i2c_error_names::bus_error,
                ::strerror(errno)
            };

        return {};
    }

    std::expected<std::byte, i2c_error> bus::read_byte(std::uint8_t command) const noexcept
    {
        return perform_i2c_read<std::byte>(
            m_bus_descriptor,
            command,
            &::i2c_smbus_read_byte_data
        );
    }

    std::expected<std::uint16_t, i2c_error> bus::read_word(std::uint8_t command)const noexcept
    {
        return perform_i2c_read<std::uint16_t>(
            m_bus_descriptor,
            command,
            &::i2c_smbus_read_word_data
        );
    }

    std::expected<std::size_t, i2c_error> bus::read_block(std::uint8_t command, std::span<std::byte> data_block) const noexcept
    {
        if (data_block.size() > I2C_SMBUS_BLOCK_MAX)
            return std::unexpected{
                i2c_error{
                    i2c_error_names::generic_error,
                    std::format("Maximum size for requested blocks is {}", I2C_SMBUS_BLOCK_MAX)
                }
            };

        const auto result = ::i2c_smbus_read_i2c_block_data(
            m_bus_descriptor,
            command,
            data_block.size(),
            reinterpret_cast<__u8*>(data_block.data()));

        if (result < 0 )
            return std::expected<std::size_t, i2c_error>{
                std::unexpect,
                i2c_error_names::transfer_error,
                ::strerror(-result)
            };

        return std::expected<std::size_t, i2c_error>{
            std::in_place,
            static_cast<std::size_t>(result)
        };
    }

    std::expected<void, i2c_error> bus::write(std::uint8_t command, std::uint8_t value) const noexcept
    {
        return perform_i2c_write(m_bus_descriptor, command, value, &::i2c_smbus_write_byte_data);
    }

    std::expected<void, i2c_error> bus::write(std::uint8_t command, std::uint16_t value) const noexcept
    {
        return perform_i2c_write(m_bus_descriptor, command, value, &::i2c_smbus_write_word_data);
    }

    std::expected<void, i2c_error> bus::write(std::uint8_t command, std::span<const std::byte> data, bool partial) const noexcept
    {
        if (data.size() > I2C_SMBUS_BLOCK_MAX && !partial)
            return std::expected<void, i2c_error>{
                std::unexpect,
                i2c_error_names::transfer_error,
                std::format("Buffer is bigger than max of {}", I2C_SMBUS_BLOCK_MAX)
            };

        const auto result = ::i2c_smbus_write_block_data(
            m_bus_descriptor,
            command,
            data.size(),
            reinterpret_cast<const __u8*>(data.data())
        );

        if (result < 0)
            return std::expected<void, i2c_error>{
                std::unexpect,
                i2c_error_names::transfer_error,
                ::strerror(-result)
            };
        else if (partial && (data.size() != static_cast<std::size_t>(result)))
            return std::expected<void, i2c_error>{
                std::unexpect,
                i2c_error_names::transfer_error,
                std::format("Failed to send the entire buffer. Expected {} sent {}", data.size(), result)
            };

        return {};
    }

}