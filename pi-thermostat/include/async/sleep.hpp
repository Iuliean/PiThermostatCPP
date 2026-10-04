#ifndef ASYNC_SLEEP_HPP
#define ASYNC_SLEEP_HPP
#include "error.hpp"
#include <bits/types/error_t.h>
#include <bits/types/sigevent_t.h>
#include <chrono>
#include <csignal>
#include <cstring>
#include <ctime>
#include <expected>
#include <memory>
#include <stdexcept>
#include <sys/time.h>
#include <system_error>
#include <time.h>

#include "coro/event.hpp"

namespace pi::async
{
    class sleep_timer
    {
    public:
        template<typename Rep, typename Period>
        static std::expected<sleep_timer, os_error> create_timer(std::chrono::duration<Rep, Period> duration)
        {
            timer_t timer_id = 0;
            auto coro_event = std::make_shared<coro::event>();

            sigevent event{};
            event.sigev_notify = SIGEV_THREAD;
            event.sigev_notify_function = &sleep_timer::timer_entry;
            event.sigev_value.sival_ptr = new std::shared_ptr{coro_event};

            const auto result = ::timer_create(
                CLOCK_MONOTONIC,
                &event,
                &timer_id
            );

            if (result == -1)
                return os_error::make_error(
                    error_t{errno},
                    std::strerror(errno)
                );

            itimerspec time{};
            const auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration);
            const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - secs);

            time.it_value.tv_sec = secs.count();
            time.it_value.tv_nsec = nsecs.count();

            const auto arm_result = ::timer_settime(
                timer_id,
                0,
                &time,
                nullptr
            );

            if (arm_result == -1)
                return os_error::make_error(
                    errno_t{errno},
                    std::strerror(errno)
                );
            
            return std::expected<sleep_timer, os_error>{std::in_place, timer_id, coro_event};
        }
        sleep_timer(timer_t timer_id, std::shared_ptr<coro::event> event);

        ~sleep_timer();

        coro::event::awaiter operator co_await() { return coro::event::awaiter(*m_event); }

    private:

        static void timer_entry(__sigval_t val);

        std::shared_ptr<coro::event> m_event;
        timer_t m_timer_id;
    };


    template<typename Rep, typename Period>
    coro::task<void> sleep_for(std::chrono::duration<Rep, Period> duration)
    {
        auto timer = sleep_timer::create_timer(duration);

        if (!timer)
            throw std::runtime_error{timer.error().message()};

        co_await *timer;
        co_return;
    }
}

#endif //ASYNC_SLEEP_HPP