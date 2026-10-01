#include "relay.hpp"
#include <gpiod.hpp>

namespace pi
{
    relay::relay(gpiod::chip& chip, std::size_t pin)
        : m_pin(pin),
          m_lines([&chip, pin](){
            return chip.prepare_request()
                .add_line_settings(
                    pin,
                    gpiod::line_settings{}.set_direction(gpiod::line::direction::OUTPUT)
                )
                .do_request();
          }())
    { }

    void relay::set(bool value)
    {
        m_lines.set_value(m_pin, value ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);
    }

    bool relay::state()
    {
        return m_lines.get_value(m_pin) == gpiod::line::value::ACTIVE ? true : false;
    }
}