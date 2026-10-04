#ifndef DISPLAY_HPP
#define DISPLAY_HPP
#include <atomic>
#include <flat_map>

#include <gpiod.hpp>
#include "coro/coro.hpp"
namespace pi
{   
    class display
    {
    public:
        display(gpiod::chip& gpio_chip, std::flat_map<char, std::size_t> segments_pin_map, std::array<std::size_t, 4> digit_pins);

        display(const display&) = delete;
        display& operator=(const display&) = delete;

        display(display&&) = delete;
        display& operator=(display&&) = delete;

        ~display();

        coro::task<void> run();

        void show(std::int32_t number);
        void set_refresh_rate(std::size_t refresh_rate_hz);
        void stop();
    private:

        void clear_segements();

        std::atomic_bool m_stop;
        std::atomic_flag m_stop_flag;

        std::flat_map<char, std::size_t> m_pin_map;
        std::array<std::size_t, 4> m_digit_pins;

        std::atomic_int32_t m_number;
        std::atomic_size_t m_refresh_rate_hz;

        gpiod::line_request m_segment_request;
        gpiod::line_request m_digit_request;
    };
}

#endif //DISPLAY_HPP