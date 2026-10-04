#include "display.hpp"
#include "async/sleep.hpp"

#include "spdlog/spdlog.h"

#include <cassert>
#include <cstdint>
#include <gpiod.hpp>
#include <ranges>
#include <array>
#include <utility>
namespace pi
{

    static const std::array<std::vector<char>,12> g_number_segments{
        std::vector{'A','B','C','E','F','D'},
        {'B','C'},
        {'A','B','G','E','D'},
        {'A','B','G','C','D'},
        {'F','B','G','C'},
        {'A','F','G','C','D'},
        {'F','E','D','C','G','A'},
        {'A','B','C'},
        {'A','B','C','D','E','F','G'},
        {'A','B','C','F','G','D'},
        {'G'},
        {},

    };

    constexpr std::span<const char> digit_to_segemnts(char digit)
    {
        if (digit == '-')
            return g_number_segments[10];
        else if (digit == ' ')
            return g_number_segments.back();
        else if (digit >= '0' && digit <= '9')
            return g_number_segments[digit - '0'];
        std::unreachable();
    }

    constexpr char to_char(std::int8_t number) noexcept
    {
        return '0' + std::abs(static_cast<int>(number));
    }

    display::display(gpiod::chip& gpio_chip, std::flat_map<char, std::size_t> segments_pin_map, std::array<std::size_t, 4> digit_pins)
        : m_stop(false),
          m_stop_flag(false),
          m_pin_map(std::move(segments_pin_map)),
          m_digit_pins(digit_pins),
          m_number(0),
          m_refresh_rate_hz(60),
          m_segment_request([this, &gpio_chip](){
            return gpio_chip.prepare_request()
                .add_line_settings(
                    std::ranges::to<gpiod::line::offsets>(m_pin_map | std::views::transform([](auto pair) {return pair.second; })),
                    gpiod::line_settings{}
                        .set_direction(::gpiod::line::direction::OUTPUT)
                )
                .do_request();
          }()),
          m_digit_request([this, &gpio_chip](){
            return gpio_chip.prepare_request()
                .add_line_settings(
                    std::ranges::to<gpiod::line::offsets>(m_digit_pins),
                    gpiod::line_settings{}
                        .set_direction(::gpiod::line::direction::OUTPUT)
                        .set_active_low(true)
                )
                .do_request();
          }())
    {}

    display::~display()
    {
        stop();
    }

    coro::task<void> display::run()
    {
        using namespace std::chrono_literals;

        while(!m_stop)
        {
            std::size_t position_shift = 0;

            const std::int32_t number = m_number;
            std::array digits{
                to_char(number % 10),
                to_char((number / 10) % 10 ),
                to_char(number / 100),
                number < 0 ? '-' : ' '
            };

            for(const auto digit : digits)
            {

                clear_segements();

                for(const char segment : digit_to_segemnts(digit))
                {
                    m_segment_request.set_value(
                        m_pin_map.at(segment),
                        ::gpiod::line::value::ACTIVE
                    );
                }

                //we are working with tenths precision
                if(position_shift == 1)
                {
                    m_segment_request.set_value(
                        m_pin_map.at('.'),
                        ::gpiod::line::value::ACTIVE
                    );
                }

                m_digit_request.set_value(
                    m_digit_pins[position_shift],
                    ::gpiod::line::value::ACTIVE
                );

                co_await async::sleep_for(2ms);

                m_digit_request.set_value(
                    m_digit_pins[position_shift],
                    ::gpiod::line::value::INACTIVE
                );

                position_shift++;
            }
            co_await async::sleep_for(1000ms / m_refresh_rate_hz.load());
        }
        spdlog::debug("Dispaly loop exit.");
        const bool prev_value = m_stop_flag.test_and_set();
        //if the flag was already true that is very dubious
        if (prev_value)
            assert(true);
        m_stop_flag.notify_all();
    }

    void display::show(std::int32_t number)
    {
        assert(number > -1000 && number < 1000);
        m_number = number;
    }

    void display::set_refresh_rate(std::size_t refresh_rate_hz)
    {
        m_refresh_rate_hz = refresh_rate_hz;
    }

    void display::stop()
    {
        m_stop = true;
        m_stop_flag.wait(false);
    }

    /**********
    * PRIVATE *
    **********/

    void display::clear_segements()
    {
        m_segment_request.set_values(::gpiod::line::values{m_pin_map.size(), ::gpiod::line::value::INACTIVE});
    }

}