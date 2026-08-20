#include "i2c/i2c_bus.hpp"
#include "i2c/i2c_device.hpp"
#include <cstddef>
#include <print>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
int main()
{
    auto bus = i2c::bus::open("/dev/i2c-1");
    auto device = i2c::device(bus.value(), i2c::address{0x76});

    auto result = device.read_byte(0xD0);


    if (!result)
        std::println("Err:{}", result.error().message());
    else
        std::println("Chip id:{}", std::to_integer<int>(result.value()));
    return 0;
}