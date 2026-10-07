#ifndef RELAY_HPP
#define RELAY_HPP
#include <gpiod.hpp>

namespace pi
{
    class relay
    {
    public:
        relay(gpiod::chip& chip, std::size_t pin);

        relay(relay&& other) noexcept;
        relay& operator=(relay&& other) noexcept;

        ~relay();

        void set(bool value);
        bool state();

    private:
        std::size_t m_pin;
        gpiod::line_request m_lines;
    };
}

#endif //RELAY_HPP